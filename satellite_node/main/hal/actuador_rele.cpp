// actuador_rele.cpp
#include "actuador_rele.h"
#include "esp_log.h"

static const char* TAG = "HAL_RELE";

ActuadorRele::ActuadorRele(gpio_num_t pin) 
    : m_pin(pin), m_estado_actual(EstadoAccion::APAGADO) {}

bool ActuadorRele::inicializar() {
    ESP_LOGI(TAG, "Configurando pin GPIO %d como salida para Relé...", static_cast<int>(m_pin));
    
    // Configuración nativa del driver GPIO
    esp_err_t err = gpio_reset_pin(m_pin);
    if (err != ESP_OK) return false;

    err = gpio_set_direction(m_pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) return false;

    // Estado inicial apagado (nivel bajo)
    err = gpio_set_level(m_pin, 0);
    if (err != ESP_OK) return false;

    m_estado_actual = EstadoAccion::APAGADO;
    return true;
}

void ActuadorRele::ejecutarAccion(EstadoAccion accion) {
    esp_err_t err = ESP_OK;

    switch (accion) {
        case EstadoAccion::ENCENDIDO:
            err = gpio_set_level(m_pin, 1);
            if (err == ESP_OK) {
                m_estado_actual = EstadoAccion::ENCENDIDO;
                ESP_LOGI(TAG, "[GPIO %d] Estado cambiado a ENCENDIDO", static_cast<int>(m_pin));
            } else {
                m_estado_actual = EstadoAccion::FALLA;
                ESP_LOGE(TAG, "[GPIO %d] Error al cambiar a ENCENDIDO", static_cast<int>(m_pin));
            }
            break;

        case EstadoAccion::APAGADO:
            err = gpio_set_level(m_pin, 0);
            if (err == ESP_OK) {
                m_estado_actual = EstadoAccion::APAGADO;
                ESP_LOGI(TAG, "[GPIO %d] Estado cambiado a APAGADO", static_cast<int>(m_pin));
            } else {
                m_estado_actual = EstadoAccion::FALLA;
                ESP_LOGE(TAG, "[GPIO %d] Error al cambiar a APAGADO", static_cast<int>(m_pin));
            }
            break;

        case EstadoAccion::FALLA:
            // Apagar por seguridad en caso de recibir comando de falla
            gpio_set_level(m_pin, 0);
            m_estado_actual = EstadoAccion::FALLA;
            ESP_LOGW(TAG, "[GPIO %d] Estado forzado a FALLA (Salida desactivada)", static_cast<int>(m_pin));
            break;
    }
}
