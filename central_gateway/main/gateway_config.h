#pragma once

namespace Gateway::Config {

constexpr const char* WIFI_SSID = "TU_WIFI_SSID_AQUI";
constexpr const char* WIFI_PASSWORD = "TU_WIFI_PASSWORD_AQUI";

constexpr const char* MQTT_BROKER_URI = "mqtt://broker.hivemq.com";
constexpr const char* MQTT_COMMAND_TOPIC = "domotica/gateway/cmd";
constexpr const char* MQTT_TELEMETRY_TOPIC_FORMAT = "domotica/nodos/%s/status";

} // namespace Gateway::Config
