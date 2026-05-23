// main.cpp - Nodo Satélite Universal
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "button_input.h"
#include "fsm_node.h"
#include "led_rgb_ws2812.h"
#include "node_config.h"
#include "spi_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "NOD_MAIN";

static LedRgbWs2812 s_led_rgb(Node::Config::RGB_LED_PIN);
static ButtonInput s_boton(Node::Config::BUTTON_PIN, Node::Config::BUTTON_ACTIVE_LOW);
static SpiSensor s_sensor_spi(Node::Config::SPI_HOST,
                              Node::Config::SPI_MOSI_PIN,
                              Node::Config::SPI_MISO_PIN,
                              Node::Config::SPI_SCLK_PIN,
                              Node::Config::SPI_CS_PIN);

// Instancia global de la Máquina de Estados Finitos
static FsmNode s_fsm(&s_led_rgb, &s_boton, &s_sensor_spi);

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Nodo Satélite Modular...");

    // 1. Inicializar NVS (Almacenamiento No Volátil). Requerido por el controlador de Wi-Fi.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicializar loop de eventos
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. Inicializar el stack de red y Wi-Fi en modo Station, pero sin conectarnos a ningún AP.
    // ESP-NOW requiere que la interfaz de Wi-Fi esté iniciada y activa.
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Crear netif por defecto para Station
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == nullptr) {
        ESP_LOGE(TAG, "Fallo al crear interfaz netif.");
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 4. Iniciar la Máquina de Estados Finitos (FSM)
    // El estado STATE_INIT de la FSM se encargará de inicializar s_rele y el stack ESP-NOW
    s_fsm.iniciar();

    ESP_LOGI(TAG, "Nodo Satélite Modular iniciado de manera exitosa y ejecutando FSM.");

    // Hilo principal se suspende de forma permanente
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
