// gateway_espnow.cpp
#include "gateway_espnow.h"
#include "gateway_mqtt.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "GTW_NOW";
static QueueHandle_t s_espnow_queue = nullptr;
static TaskHandle_t s_processor_task_handle = nullptr;

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
}

// Callback nativo de transmisión de datos (ESP-IDF v6.x firma de callback)
static void espnow_send_cb(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    if (tx_info == nullptr) return;
    
    const uint8_t* mac_addr = tx_info->ra;
    ESP_LOGI(TAG, "Envío ESP-NOW a %02x:%02x:%02x:%02x:%02x:%02x completado. Estado: %s",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5],
             status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Fallo");
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
        peer_info.channel = 0; // Canal actual de Wi-Fi STA
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
    return err;
}

} // namespace Gateway::Espnow
