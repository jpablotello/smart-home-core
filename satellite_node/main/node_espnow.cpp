// node_espnow.cpp
#include "node_espnow.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "NOD_NOW";
static QueueHandle_t s_rx_queue = nullptr;

static uint8_t s_gateway_mac[6] = {0};
static bool s_gateway_registered = false;

namespace Node::Espnow {

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
static void espnow_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
    if (mac_addr == nullptr) return;
    ESP_LOGD(TAG, "Reporte enviado a %02x:%02x:%02x:%02x:%02x:%02x. Estado: %s",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5],
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

    ESP_LOGI(TAG, "ESP-NOW del Nodo inicializado con éxito.");
    return ESP_OK;
}

QueueHandle_t get_rx_queue() {
    return s_rx_queue;
}

esp_err_t send_report(const DomoMessage_t& msg) {
    if (!s_gateway_registered) {
        ESP_LOGW(TAG, "No hay Central registrada. Imposible enviar reporte.");
        return ESP_ERR_INVALID_STATE;
    }

    return esp_now_send(s_gateway_mac, reinterpret_cast<const uint8_t*>(&msg), sizeof(DomoMessage_t));
}

bool get_gateway_mac(uint8_t* mac_out) {
    if (s_gateway_registered && mac_out != nullptr) {
        memcpy(mac_out, s_gateway_mac, 6);
        return true;
    }
    return false;
}

} // namespace Node::Espnow
