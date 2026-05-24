// gateway_espnow.cpp
#include "gateway_espnow.h"
#include "gateway_mqtt.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <atomic>
#include <string.h>

static const char* TAG = "GTW_NOW";
static QueueHandle_t s_espnow_queue = nullptr;
static TaskHandle_t s_processor_task_handle = nullptr;
static TaskHandle_t s_beacon_task_handle = nullptr;
static std::atomic<bool> s_has_peers{false};

static const uint8_t BROADCAST_MAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

namespace Gateway::Espnow {

// Tarea en segundo plano para procesar tramas de ESP-NOW y subirlas a MQTT
static void espnow_processor_task(void* pvParameters) {
    DomoMessage_t msg;
    ESP_LOGI(TAG, "Tarea de procesamiento ESP-NOW iniciada.");
    
    while (true) {
        if (xQueueReceive(s_espnow_queue, &msg, portMAX_DELAY) == pdTRUE) {
            // Log limpio por consola (ESP_LOGI)
            ESP_LOGI(TAG, "--------------------------------------------------");
            ESP_LOGI(TAG, "Tramas recibidas de Nodo: %02x:%02x:%02x:%02x:%02x:%02x",
                     msg.mac_origen[0], msg.mac_origen[1], msg.mac_origen[2],
                     msg.mac_origen[3], msg.mac_origen[4], msg.mac_origen[5]);
            ESP_LOGI(TAG, "Tipo Nodo         : %d", static_cast<int>(msg.tipo_nodo));
            ESP_LOGI(TAG, "Pin Afectado      : %d", msg.pin_afectado);
            ESP_LOGI(TAG, "Estado Reportado  : %d", msg.estado_solicitado);
            ESP_LOGI(TAG, "LED Brightness    : %d%%", msg.led_brightness);
            ESP_LOGI(TAG, "Button Pressed    : %s", msg.button_pressed ? "YES" : "NO");
            ESP_LOGI(TAG, "SPI Value         : %d", msg.spi_value);
            ESP_LOGI(TAG, "Temp Lectura      : %.2f °C", msg.lectura_temperatura);
            ESP_LOGI(TAG, "Hum Lectura       : %.2f %%", msg.lectura_humedad);
            ESP_LOGI(TAG, "Timestamp Op      : %lu ms", msg.timestamp_operacion);
            ESP_LOGI(TAG, "--------------------------------------------------");

            // Reenvío de datos a MQTT
            esp_err_t err = Gateway::Mqtt::publish_sensor_data(msg);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Fallo al reenviar datos del nodo a MQTT (error: %s)", esp_err_to_name(err));
            }
        }
    }
}

// Callback nativo de recepción de datos (ESP-IDF v5.x firma de callback)
static void espnow_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    if (recv_info == nullptr || data == nullptr) {
        return;
    }

    if (len != sizeof(DomoMessage_t)) {
        ESP_LOGW(TAG, "Tamaño de trama inválido: recibido %d bytes, esperado %d", len, sizeof(DomoMessage_t));
        return;
    }

    DomoMessage_t msg;
    memcpy(&msg, data, sizeof(DomoMessage_t));
    
    // Sobrescribimos la MAC de origen con la MAC real del emisor obtenida del stack
    memcpy(msg.mac_origen, recv_info->src_addr, 6);

    // Enviar a la cola de procesamiento
    if (s_espnow_queue != nullptr) {
        if (xQueueSend(s_espnow_queue, &msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Cola de procesamiento ESP-NOW llena. Trama descartada.");
        }
    }
    
    // Registrar peer dinámicamente si no existe (permite responder y mantener estado)
    if (!esp_now_is_peer_exist(recv_info->src_addr)) {
        esp_now_peer_info_t peer_info = {};
        memcpy(peer_info.peer_addr, recv_info->src_addr, 6);
        // Intentar tomar el canal desde rx_ctrl si está disponible
        if (recv_info->rx_ctrl != nullptr) {
            peer_info.channel = recv_info->rx_ctrl->channel;
        } else {
            uint8_t primary_chan = 0;
            wifi_second_chan_t second_chan = WIFI_SECOND_CHAN_NONE;
            if (esp_wifi_get_channel(&primary_chan, &second_chan) == ESP_OK) {
                peer_info.channel = primary_chan;
            } else {
                peer_info.channel = 0;
            }
        }
        peer_info.ifidx = WIFI_IF_STA;
        peer_info.encrypt = false;

        esp_err_t perr = esp_now_add_peer(&peer_info);
        if (perr == ESP_OK) {
            ESP_LOGI(TAG, "Peer agregado dinámicamente: %02x:%02x:%02x:%02x:%02x:%02x (canal=%d)",
                     recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                     recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5], peer_info.channel);
            s_has_peers.store(true);
        } else {
            ESP_LOGW(TAG, "Fallo al agregar peer dinámico: %s", esp_err_to_name(perr));
        }
    } else {
        s_has_peers.store(true);
    }
    
    // Enviar un ACK rápido al nodo emisor para que pueda detectar y registrar la Central
    DomoMessage_t ack = {};
    esp_wifi_get_mac(WIFI_IF_STA, ack.mac_origen);
    ack.tipo_nodo = TipoNodo::HIBRIDO;
    ack.pin_afectado = 0;
    ack.estado_solicitado = 255; // código especial de ACK
    esp_err_t ack_err = esp_now_send(recv_info->src_addr, reinterpret_cast<uint8_t*>(&ack), sizeof(ack));
    if (ack_err != ESP_OK) {
        ESP_LOGW(TAG, "Fallo al enviar ACK al nodo %02x:%02x:%02x:%02x:%02x:%02x: %s",
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5], esp_err_to_name(ack_err));
    } else {
        ESP_LOGD(TAG, "ACK enviado al nodo %02x:%02x:%02x:%02x:%02x:%02x",
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
    }
}

// Callback nativo de transmisión de datos (ESP-IDF v6.x firma de callback)
static void espnow_send_cb(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    if (tx_info == nullptr) return;
    
    // El campo correcto para la dirección de destino es des_addr en esta versión de ESP-IDF.
    const uint8_t* mac_addr = tx_info->des_addr;
    if (mac_addr == nullptr) return;

    ESP_LOGI(TAG, "Envío ESP-NOW a %02x:%02x:%02x:%02x:%02x:%02x completado. Estado: %s (code=%d)",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5],
             status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Fallo",
             static_cast<int>(status));
}

esp_err_t init() {
    // 1. Inicializar ESP-NOW
    esp_err_t err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar ESP-NOW: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Registrar Callbacks
    err = esp_now_register_recv_cb(espnow_recv_cb);
    if (err != ESP_OK) return err;

    err = esp_now_register_send_cb(espnow_send_cb);
    if (err != ESP_OK) return err;

    // 3. Crear la cola de FreeRTOS para procesamiento
    s_espnow_queue = xQueueCreate(10, sizeof(DomoMessage_t));
    if (s_espnow_queue == nullptr) {
        ESP_LOGE(TAG, "Fallo al crear cola de procesamiento.");
        return ESP_ERR_NO_MEM;
    }

    // 4. Crear tarea de FreeRTOS
    BaseType_t task_err = xTaskCreate(espnow_processor_task,
                                      "espnow_proc_task",
                                      4096,
                                      nullptr,
                                      5,
                                      &s_processor_task_handle);
    if (task_err != pdPASS) {
        ESP_LOGE(TAG, "Fallo al crear la tarea de procesamiento.");
        vQueueDelete(s_espnow_queue);
        s_espnow_queue = nullptr;
        return ESP_FAIL;
    }

    // Añadir peer broadcast para poder enviar/recibir broadcasts desde/hacia nodos
    if (!esp_now_is_peer_exist(BROADCAST_MAC)) {
        esp_now_peer_info_t broadcast_peer = {};
        memcpy(broadcast_peer.peer_addr, BROADCAST_MAC, 6);
        broadcast_peer.channel = 0; // usar canal activo
        broadcast_peer.ifidx = WIFI_IF_STA;
        broadcast_peer.encrypt = false;
        esp_err_t peer_err = esp_now_add_peer(&broadcast_peer);
        if (peer_err != ESP_OK) {
            ESP_LOGW(TAG, "No se pudo añadir peer broadcast: %s", esp_err_to_name(peer_err));
        }
    }

    // Crear tarea temporal que emite beacons broadcast para facilitar discovery
    BaseType_t beacon_task_err = xTaskCreate([](void*){
        DomoMessage_t beacon = {};
        esp_wifi_get_mac(WIFI_IF_STA, beacon.mac_origen);
        beacon.tipo_nodo = TipoNodo::HIBRIDO;
        beacon.estado_solicitado = 254; // codigo de beacon

        while (true) {
            if (!s_has_peers.load()) {
                // Emitir ráfaga de beacons durante ~5 segundos (250ms interval)
                for (int i = 0; i < 20; ++i) {
                    esp_err_t send_err = esp_now_send(BROADCAST_MAC, reinterpret_cast<uint8_t*>(&beacon), sizeof(beacon));
                    if (send_err != ESP_OK) {
                        ESP_LOGD(TAG, "Beacon broadcast fallo: %s", esp_err_to_name(send_err));
                    }
                    vTaskDelay(pdMS_TO_TICKS(250));
                }
                // Esperar antes de la siguiente ventana de beacons
                vTaskDelay(pdMS_TO_TICKS(30000)); // 30s
            } else {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        vTaskDelete(nullptr);
    }, "espnow_beacon_task", 4096, nullptr, 5, &s_beacon_task_handle);
    if (beacon_task_err != pdPASS) {
        ESP_LOGW(TAG, "No se pudo crear la tarea de beacons.");
    }

    ESP_LOGI(TAG, "Módulo ESP-NOW y tareas asociadas inicializadas correctamente.");
    return ESP_OK;
}

esp_err_t send_command(const uint8_t* dest_mac, uint8_t pin, uint8_t state) {
    if (dest_mac == nullptr) return ESP_ERR_INVALID_ARG;

    // Comprobar si el dispositivo ya está registrado como par (peer)
    if (!esp_now_is_peer_exist(dest_mac)) {
        ESP_LOGI(TAG, "El peer no existe en la lista. Registrando dinámicamente...");
        esp_now_peer_info_t peer_info = {};
        memcpy(peer_info.peer_addr, dest_mac, 6);
        // Intentar obtener el canal Wi‑Fi actual de la interfaz STA y asignarlo al peer
        uint8_t primary_chan = 0;
        wifi_second_chan_t second_chan = WIFI_SECOND_CHAN_NONE;
        if (esp_wifi_get_channel(&primary_chan, &second_chan) == ESP_OK) {
            peer_info.channel = primary_chan;
            ESP_LOGI(TAG, "Registrando peer en canal Wi-Fi: %u", primary_chan);
        } else {
            peer_info.channel = 0; // fallback: permitir que el stack intente
            ESP_LOGW(TAG, "No se pudo obtener canal Wi‑Fi actual; usando channel=0 para el peer.");
        }
        peer_info.ifidx = WIFI_IF_STA;
        peer_info.encrypt = false;
        
        esp_err_t err = esp_now_add_peer(&peer_info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Fallo al registrar el peer: %s", esp_err_to_name(err));
            return err;
        }
    }

    // Construir mensaje DomoMessage_t
    DomoMessage_t msg = {};
    esp_wifi_get_mac(WIFI_IF_STA, msg.mac_origen);
    msg.tipo_nodo = TipoNodo::HIBRIDO; // Se autoidentifica el gateway como Híbrido
    msg.pin_afectado = pin;
    msg.estado_solicitado = state;
    msg.lectura_temperatura = 0.0f;
    msg.lectura_humedad = 0.0f;
    msg.timestamp_operacion = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Despachar comando por ESP-NOW
    esp_err_t err = esp_now_send(dest_mac, reinterpret_cast<uint8_t*>(&msg), sizeof(DomoMessage_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_send falló inmediatamente: %s", esp_err_to_name(err));
    }
    return err;
}

} // namespace Gateway::Espnow
