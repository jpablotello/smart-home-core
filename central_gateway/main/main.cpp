// main.cpp - Gateway Central
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "gateway_wifi.h"
#include "gateway_espnow.h"
#include "gateway_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "GTW_MAIN";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Gateway Central de Domótica...");

    // 1. Inicializar NVS (Almacenamiento No Volátil). Requerido por el stack de Wi-Fi.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicializar el loop de eventos por defecto de ESP-IDF
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. Inicializar Wi-Fi en modo Station
    // NOTA: Configura tus credenciales reales aquí.
    const char* wifi_ssid = "TU_WIFI_SSID_AQUI";
    const char* wifi_pass = "TU_WIFI_PASSWORD_AQUI";
    ESP_LOGI(TAG, "Configurando Wi-Fi (SSID: %s)...", wifi_ssid);
    ESP_ERROR_CHECK(Gateway::Wifi::init_sta(wifi_ssid, wifi_pass));

    // 4. Inicializar módulo ESP-NOW
    // Se debe llamar una vez que Wi-Fi esté inicializado (modo STA activo)
    ESP_LOGI(TAG, "Configurando protocolo ESP-NOW...");
    ESP_ERROR_CHECK(Gateway::Espnow::init());

    // 5. Inicializar el cliente MQTT
    // Se utiliza un Broker público para pruebas por defecto, cámbialo a tu Broker local (ej: mosquitto) en producción.
    const char* mqtt_broker = "mqtt://broker.hivemq.com";
    ESP_LOGI(TAG, "Configurando MQTT broker: %s...", mqtt_broker);
    ESP_ERROR_CHECK(Gateway::Mqtt::init(mqtt_broker));

    ESP_LOGI(TAG, "Gateway Central inicializado de manera exitosa.");

    // Loop de monitorización principal
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(15000));
        bool connected = Gateway::Wifi::is_connected();
        ESP_LOGI(TAG, "[MONITOR] Estado de red -> Wi-Fi: %s", 
                 connected ? "CONECTADO" : "DESCONECTADO");
    }
}
