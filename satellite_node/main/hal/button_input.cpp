#include "button_input.h"
#include "esp_log.h"

static const char* TAG = "BUTTON_IN";

ButtonInput::ButtonInput(gpio_num_t pin, bool active_low)
    : m_pin(pin), m_active_low(active_low), m_inicializado(false) {}

bool ButtonInput::inicializar() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << static_cast<int>(m_pin));
    io_conf.pull_down_en = m_active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = m_active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button pin %d", static_cast<int>(m_pin));
        m_inicializado = false;
        return false;
    }

    m_inicializado = true;
    ESP_LOGI(TAG, "Button input initialized on GPIO %d", static_cast<int>(m_pin));
    return true;
}

bool ButtonInput::estaActiva() const {
    if (!m_inicializado) {
        return false;
    }
    int level = gpio_get_level(m_pin);
    return m_active_low ? (level == 0) : (level == 1);
}
