// node_espnow.cpp
#include "node_espnow.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char* TAG = "NOD_NOW";
static QueueHandle_t s_rx_queue = nullptr;

static uint8_t s_gateway_mac[6] = {0};
static bool s_gateway_registered = false;
static constexpr uint8_t BROADCAST_MAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

namespace Node::Espnow {

static void save_gateway_info(const uint8_t* mac, uint8_t channel) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("espnow", NVS_READWRITE, &h);
    if (err != ESP_OK) return;
    err = nvs_set_blob(h, "gateway_mac", mac, 6);
    if (err == ESP_OK) err = nvs_set_u8(h, "gateway_chan", channel);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
}

static bool load_gateway_info(uint8_t* mac_out, uint8_t* channel_out) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("espnow", NVS_READONLY, &h);
    if (err != ESP_OK) return false;
    size_t required = 6;
    err = nvs_get_blob(h, "gateway_mac", mac_out, &required);
    if (err != ESP_OK || required != 6) {
        nvs_close(h);
        return false;
    }
    uint8_t ch = 0;
    err = nvs_get_u8(h, "gateway_chan", &ch);
    nvs_close(h);
    if (err != ESP_OK) return false;
    *channel_out = ch;
    return true;
}

// Callback nativo de recepción (ESP-IDF v5.x)
static void espnow_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    if (recv_info == nullptr || data == nullptr) {
        return;
    }

    if (len != sizeof(DomoMessage_t)) {
        ESP_LOGW(TAG, "Tamaño de trama inválido: %d bytes (esperado %d)", len, sizeof(DomoMessage_t));
        return;
    }

    // Registro dinámico de la Central Gateway al recibir el primer paquete
    if (!s_gateway_registered || memcmp(s_gateway_mac, recv_info->src_addr, 6) != 0) {
        ESP_LOGI(TAG, "Nueva Central detectada en MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);

        // Log del canal en que se recibió la trama (útil para diagnóstico de ESP-NOW)
        if (recv_info->rx_ctrl != nullptr) {
            ESP_LOGI(TAG, "Paquete recibido en canal: %d", recv_info->rx_ctrl->channel);
        } else {
            ESP_LOGI(TAG, "Paquete recibido (rx_ctrl == nullptr)");
        }

        // Si ya había una central pero cambió de dirección, removemos la anterior
        if (s_gateway_registered) {
            esp_now_del_peer(s_gateway_mac);
        }

        // Registrar la central como Peer para poder responderle
        esp_now_peer_info_t peer_info = {};
        memcpy(peer_info.peer_addr, recv_info->src_addr, 6);
        peer_info.channel = 0; // Utilizar canal actual
        peer_info.encrypt = false;

        esp_err_t err = esp_now_add_peer(&peer_info);
        if (err == ESP_OK) {
            memcpy(s_gateway_mac, recv_info->src_addr, 6);
            s_gateway_registered = true;
            ESP_LOGI(TAG, "Central registrada como par (peer) de ESP-NOW con éxito.");
            // Guardar en NVS para arranques futuros
            uint8_t chan = (recv_info->rx_ctrl != nullptr) ? recv_info->rx_ctrl->channel : 0;
            save_gateway_info(s_gateway_mac, chan);
        } else {
            ESP_LOGE(TAG, "Fallo al registrar Central como peer (error: %s)", esp_err_to_name(err));
        }
    }

    // Copiar mensaje y encolar
    DomoMessage_t msg;
    memcpy(&msg, data, sizeof(DomoMessage_t));
    memcpy(msg.mac_origen, recv_info->src_addr, 6); // Asegurar MAC de origen

    if (s_rx_queue != nullptr) {
        if (xQueueSend(s_rx_queue, &msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Cola de comandos llena. Trama descartada.");
        }
    }
}

// Callback de envío completado
static void espnow_send_cb(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    if (tx_info == nullptr || tx_info->des_addr == nullptr) return;
    ESP_LOGD(TAG, "Reporte enviado a %02x:%02x:%02x:%02x:%02x:%02x. Estado: %s",
             tx_info->des_addr[0], tx_info->des_addr[1], tx_info->des_addr[2],
             tx_info->des_addr[3], tx_info->des_addr[4], tx_info->des_addr[5],
             status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Fallo");
}

esp_err_t init() {
    s_rx_queue = xQueueCreate(5, sizeof(DomoMessage_t));
    if (s_rx_queue == nullptr) {
        ESP_LOGE(TAG, "Fallo al crear cola de recepción de comandos.");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando ESP-NOW: %s", esp_err_to_name(err));
        vQueueDelete(s_rx_queue);
        s_rx_queue = nullptr;
        return err;
    }

    err = esp_now_register_recv_cb(espnow_recv_cb);
    if (err != ESP_OK) return err;

    err = esp_now_register_send_cb(espnow_send_cb);
    if (err != ESP_OK) return err;

    // Añadir peer broadcast (canal 0 -> usar canal actual al transmitir)
    if (!esp_now_is_peer_exist(BROADCAST_MAC)) {
        esp_now_peer_info_t broadcast_peer = {};
        memcpy(broadcast_peer.peer_addr, BROADCAST_MAC, 6);
        broadcast_peer.channel = 0;
        broadcast_peer.encrypt = false;

        err = esp_now_add_peer(&broadcast_peer);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error registrando peer broadcast: %s", esp_err_to_name(err));
            return err;
        }
    }

    // Intento: cargar información previa de la gateway desde NVS
    uint8_t saved_mac[6];
    uint8_t saved_chan = 0;
    if (load_gateway_info(saved_mac, &saved_chan)) {
        ESP_LOGI(TAG, "Info de gateway cargada desde NVS. Intentando registrar peer en canal %d", saved_chan);
        esp_now_peer_info_t peer_info = {};
        memcpy(peer_info.peer_addr, saved_mac, 6);
        peer_info.channel = saved_chan;
        peer_info.encrypt = false;
        esp_err_t perr = esp_now_add_peer(&peer_info);
        if (perr == ESP_OK) {
            memcpy(s_gateway_mac, saved_mac, 6);
            s_gateway_registered = true;
            ESP_LOGI(TAG, "Gateway registrada a partir de NVS.");
        } else {
            ESP_LOGW(TAG, "No se pudo registrar gateway desde NVS: %s", esp_err_to_name(perr));
        }
    }

    // Lanzar tarea en background que hace barridos de canales en ráfagas y con backoff
    BaseType_t sweep_err = xTaskCreate([](void*){
        while (!s_gateway_registered) {
            // Hacer 3 barridos rápidos
            for (int sweep = 0; sweep < 3 && !s_gateway_registered; ++sweep) {
                for (int ch = 1; ch <= 13 && !s_gateway_registered; ++ch) {
                    esp_err_t ch_err = esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
                    if (ch_err != ESP_OK) {
                        ESP_LOGW(TAG, "No se pudo cambiar a canal %d: %s", ch, esp_err_to_name(ch_err));
                        continue;
                    }
                    ESP_LOGI(TAG, "Probando canal %d para descubrir la Central...", ch);

                    DomoMessage_t probe = {};
                    esp_wifi_get_mac(WIFI_IF_STA, probe.mac_origen);
                    probe.tipo_nodo = TipoNodo::ACTUADOR_DIGITAL;
                    probe.estado_solicitado = 0;
                    esp_err_t send_err = esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t*>(&probe), sizeof(probe));
                    if (send_err != ESP_OK) {
                        ESP_LOGW(TAG, "esp_now_send probe en canal %d falló: %s", ch, esp_err_to_name(send_err));
                    }
                    vTaskDelay(pdMS_TO_TICKS(400));
                }
            }
            // Backoff antes de siguiente ronda de barridos
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        vTaskDelete(nullptr);
    }, "espnow_sweep_task", 4096, nullptr, 5, nullptr);
    if (sweep_err != pdPASS) {
        ESP_LOGW(TAG, "No se pudo crear la tarea de sweep de canales.");
    }

    ESP_LOGI(TAG, "ESP-NOW del Nodo inicializado con éxito.");
    return ESP_OK;
}

QueueHandle_t get_rx_queue() {
    return s_rx_queue;
}

esp_err_t send_report(const DomoMessage_t& msg) {
    const uint8_t* dest_mac = s_gateway_registered ? s_gateway_mac : BROADCAST_MAC;
    if (!s_gateway_registered) {
        ESP_LOGW(TAG, "No hay Central registrada. Enviando reporte por broadcast.");
    }

    return esp_now_send(dest_mac, reinterpret_cast<const uint8_t*>(&msg), sizeof(DomoMessage_t));
}

bool get_gateway_mac(uint8_t* mac_out) {
    if (s_gateway_registered && mac_out != nullptr) {
        memcpy(mac_out, s_gateway_mac, 6);
        return true;
    }
    return false;
}

} // namespace Node::Espnow
