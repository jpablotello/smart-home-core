// gateway_wifi.h
#pragma once

#include "esp_err.h"

namespace Gateway::Wifi {

/**
 * @brief Inicializa el driver de Wi-Fi en modo Station (STA) y comienza el proceso de conexión.
 * 
 * @param ssid Nombre de la red Wi-Fi.
 * @param password Contraseña de la red Wi-Fi.
 * @return esp_err_t ESP_OK en caso de éxito, o código de error nativo.
 */
esp_err_t init_sta(const char* ssid, const char* password);

/**
 * @brief Verifica si el Gateway se encuentra conectado y con dirección IP asignada.
 * 
 * @return true Si está conectado.
 * @return false Si no está conectado.
 */
bool is_connected();

} // namespace Gateway::Wifi
