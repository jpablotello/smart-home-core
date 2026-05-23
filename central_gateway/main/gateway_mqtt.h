// gateway_mqtt.h
#pragma once

#include "esp_err.h"
#include "domo_protocol.h"

namespace Gateway::Mqtt {

/**
 * @brief Inicializa el cliente MQTT y se conecta al Broker especificado.
 * 
 * @param broker_uri URI del broker MQTT (ej: "mqtt://broker.hivemq.com").
 * @return esp_err_t ESP_OK en caso de éxito.
 */
esp_err_t init(const char* broker_uri);

/**
 * @brief Publica la telemetría de un nodo satélite en el broker MQTT.
 * 
 * @param msg Mensaje recibido desde el satélite.
 * @return esp_err_t ESP_OK en caso de éxito.
 */
esp_err_t publish_sensor_data(const DomoMessage_t& msg);

} // namespace Gateway::Mqtt
