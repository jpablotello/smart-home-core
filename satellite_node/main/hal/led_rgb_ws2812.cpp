#include "led_rgb_ws2812.h"
#include "esp_log.h"

static const char* TAG = "LED_RGB";

LedRgbWs2812::LedRgbWs2812(gpio_num_t pin)
    : m_pin(pin), m_strip(nullptr), m_estado_actual(EstadoAccion::APAGADO) {}

bool LedRgbWs2812::inicializar() {
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = m_pin;
    strip_config.max_leds = 1;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000;
    rmt_config.flags.with_dma = false;

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &m_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar WS2812 en GPIO %d: %s",
                 static_cast<int>(m_pin), esp_err_to_name(err));
        return false;
    }

    err = led_strip_clear(m_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo apagar WS2812: %s", esp_err_to_name(err));
        return false;
    }

    m_estado_actual = EstadoAccion::APAGADO;
    ESP_LOGI(TAG, "LED RGB WS2812 inicializado en GPIO %d", static_cast<int>(m_pin));
    return true;
}

void LedRgbWs2812::ejecutarAccion(EstadoAccion accion) {
    if (m_strip == nullptr) {
        m_estado_actual = EstadoAccion::FALLA;
        return;
    }

    switch (accion) {
        case EstadoAccion::ENCENDIDO:
            setColor(0, 32, 0);
            m_estado_actual = EstadoAccion::ENCENDIDO;
            break;
        case EstadoAccion::APAGADO:
            led_strip_clear(m_strip);
            m_estado_actual = EstadoAccion::APAGADO;
            break;
        case EstadoAccion::FALLA:
            setColor(32, 0, 0);
            m_estado_actual = EstadoAccion::FALLA;
            break;
    }
}

void LedRgbWs2812::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    if (m_strip == nullptr) {
        return;
    }

    esp_err_t err = led_strip_set_pixel(m_strip, 0, red, green, blue);
    if (err == ESP_OK) {
        err = led_strip_refresh(m_strip);
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No se pudo actualizar WS2812: %s", esp_err_to_name(err));
        m_estado_actual = EstadoAccion::FALLA;
    }
}
