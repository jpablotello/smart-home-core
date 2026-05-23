// node_espnow.h
#pragma once

#include "esp_err.h"
#include "domo_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace Node::Espnow {

/**
 * @brief Inicializa el módulo ESP-NOW y crea la cola de comandos recibidos.
 * 
 * @return esp_err_t ESP_OK en caso de éxito.
 */
esp_err_t init();

/**
 * @brief Obtiene el manejador de la cola donde se encolan los comandos entrantes de la Central.
 * 
 * @return QueueHandle_t Cola de FreeRTOS que contiene estructuras DomoMessage_t.
 */
QueueHandle_t get_rx_queue();

/**
 * @brief Envía el reporte de telemetría de vuelta a la Central.
 * 
 * @param msg Estructura con las lecturas y estados actuales.
 * @return esp_err_t ESP_OK en caso de éxito.
 */
esp_err_t send_report(const DomoMessage_t& msg);

/**
 * @brief Obtiene la dirección MAC registrada actualmente de la Central.
 * 
 * @param mac_out Puntero a un array de 6 bytes donde se copiará la MAC.
 * @return true Si hay una MAC válida registrada.
 * @return false Si no hay central registrada.
 */
bool get_gateway_mac(uint8_t* mac_out);

} // namespace Node::Espnow
