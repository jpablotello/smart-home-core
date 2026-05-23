// gateway_espnow.h
#pragma once

#include "esp_err.h"
#include "domo_protocol.h"

namespace Gateway::Espnow {

/**
 * @brief Inicializa el módulo ESP-NOW, registra callbacks nativos y crea la cola de procesamiento.
 * 
 * @return esp_err_t ESP_OK en caso de éxito.
 */
esp_err_t init();

/**
 * @brief Envía un comando a un nodo satélite específico.
 * 
 * @param dest_mac Dirección MAC de 6 bytes del nodo destino.
 * @param pin Pin GPIO afectado en el nodo satélite.
 * @param state Estado solicitado (0 o 1).
 * @return esp_err_t ESP_OK en caso de éxito.
 */
esp_err_t send_command(const uint8_t* dest_mac, uint8_t pin, uint8_t state);

} // namespace Gateway::Espnow
