// gateway_wifi.cpp
#include "gateway_wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>
#include <atomic>

static const char* TAG = "GTW_WIFI";

namespace Gateway::Wifi {

static std::atomic<bool> s_connected{false};
static int s_retry_num = 0;
constexpr int MAX_RETRIES = 5;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Conectando al AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (s_retry_num < MAX_RETRIES) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Reintentando conexión al AP (%d/%d)...", s_retry_num, MAX_RETRIES);
        } else {
            ESP_LOGW(TAG, "Fallo al conectar al AP tras alcanzar el número máximo de reintentos.");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP Asignada: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_connected = true;
    }
}

esp_err_t init_sta(const char* ssid, const char* password) {
    s_connected = false;
    s_retry_num = 0;

    // Inicializar el stack TCP/IP y el loop de eventos si no lo están
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) return err;

    // Crear la interfaz de red por defecto para Station
    esp_netif_t* netif = esp_netif_create_default_wifi_sta();
    if (netif == nullptr) {
        ESP_LOGE(TAG, "Error al crear netif para STA");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) return err;

    // Registrar manejadores de eventos nativos
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    err = esp_event_handler_instance_register(WIFI_EVENT,
                                              ESP_EVENT_ANY_ID,
                                              &wifi_event_handler,
                                              nullptr,
                                              &instance_any_id);
    if (err != ESP_OK) return err;

    err = esp_event_handler_instance_register(IP_EVENT,
                                              IP_EVENT_STA_GOT_IP,
                                              &wifi_event_handler,
                                              nullptr,
                                              &instance_got_ip);
    if (err != ESP_OK) return err;

    // Configurar SSID y Password
    wifi_config_t wifi_config = {};
    // C++17 estructura inicializada de forma segura
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) return err;

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) return err;

    err = esp_wifi_start();
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Inicialización de Wi-Fi STA completada.");
    return ESP_OK;
}

bool is_connected() {
    return s_connected.load();
}

} // namespace Gateway::Wifi
