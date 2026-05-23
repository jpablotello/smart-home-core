// gateway_mqtt.cpp
#include "gateway_mqtt.h"
#include "gateway_espnow.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char* TAG = "GTW_MQTT";
static esp_mqtt_client_handle_t s_mqtt_client = nullptr;

namespace Gateway::Mqtt {

static bool parse_mac_string(const char* mac_str, uint8_t* mac_out) {
    int bytes[6];
    int parsed = sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                        &bytes[0], &bytes[1], &bytes[2],
                        &bytes[3], &bytes[4], &bytes[5]);
    if (parsed == 6) {
        for (int i = 0; i < 6; i++) {
            mac_out[i] = static_cast<uint8_t>(bytes[i]);
        }
        return true;
    }
    return false;
}

static void handle_mqtt_command(const char* data, int length) {
    ESP_LOGI(TAG, "Procesando comando recibido por MQTT: %.*s", length, data);
    
    // Parseo del JSON
    cJSON* root = cJSON_ParseWithLength(data, length);
    if (root == nullptr) {
        ESP_LOGE(TAG, "Error al parsear JSON del comando MQTT.");
        return;
    }

    cJSON* mac_item = cJSON_GetObjectItem(root, "mac");
    cJSON* pin_item = cJSON_GetObjectItem(root, "pin");
    cJSON* estado_item = cJSON_GetObjectItem(root, "estado");

    if (cJSON_IsString(mac_item) && cJSON_IsNumber(pin_item) && cJSON_IsNumber(estado_item)) {
        uint8_t dest_mac[6];
        if (parse_mac_string(mac_item->valuestring, dest_mac)) {
            uint8_t pin = static_cast<uint8_t>(pin_item->valueint);
            uint8_t state = static_cast<uint8_t>(estado_item->valueint);

            ESP_LOGI(TAG, "Ruteando comando por ESP-NOW a MAC: %s | Pin: %d | Estado: %d",
                     mac_item->valuestring, pin, state);

            // Enviar el comando por ESP-NOW
            esp_err_t err = Gateway::Espnow::send_command(dest_mac, pin, state);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Fallo al enviar comando por ESP-NOW (error: %s)", esp_err_to_name(err));
            }
        } else {
            ESP_LOGE(TAG, "Formato de MAC inválido. Debe ser XX:XX:XX:XX:XX:XX");
        }
    } else {
        ESP_LOGE(TAG, "Faltan campos obligatorios en el JSON ('mac', 'pin', 'estado')");
    }

    cJSON_Delete(root);
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base,
                               int32_t event_id, void* event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado al broker MQTT.");
            // Suscribirse al tópico de comandos
            esp_mqtt_client_subscribe(event->client, "domotica/gateway/cmd", 1);
            ESP_LOGI(TAG, "Suscrito a: domotica/gateway/cmd");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Desconectado del broker MQTT.");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "Tópico suscrito con éxito (msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            if (strncmp(event->topic, "domotica/gateway/cmd", event->topic_len) == 0) {
                handle_mqtt_command(event->data, event->data_len);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Error detectado en el cliente MQTT.");
            break;
        default:
            break;
    }
}

esp_err_t init(const char* broker_uri) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    // Estructura de ESP-IDF v5.x para configuración de MQTT
    mqtt_cfg.broker.address.uri = broker_uri;

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == nullptr) {
        ESP_LOGE(TAG, "No se pudo inicializar el cliente MQTT.");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(s_mqtt_client, 
                                                   static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), 
                                                   mqtt_event_handler, 
                                                   nullptr);
    if (err != ESP_OK) return err;

    err = esp_mqtt_client_start(s_mqtt_client);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Cliente MQTT iniciado para URI: %s", broker_uri);
    return ESP_OK;
}

esp_err_t publish_sensor_data(const DomoMessage_t& msg) {
    if (s_mqtt_client == nullptr) {
        ESP_LOGW(TAG, "El cliente MQTT no está inicializado.");
        return ESP_ERR_INVALID_STATE;
    }

    // Convertir MAC origen a string para el JSON
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             msg.mac_origen[0], msg.mac_origen[1], msg.mac_origen[2],
             msg.mac_origen[3], msg.mac_origen[4], msg.mac_origen[5]);

    // Crear JSON
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mac_origen", mac_str);
    cJSON_AddNumberToObject(root, "tipo_nodo", static_cast<int>(msg.tipo_nodo));
    cJSON_AddNumberToObject(root, "pin_afectado", msg.pin_afectado);
    cJSON_AddNumberToObject(root, "estado_solicitado", msg.estado_solicitado);
    cJSON_AddNumberToObject(root, "temperatura", msg.lectura_temperatura);
    cJSON_AddNumberToObject(root, "humedad", msg.lectura_humedad);
    cJSON_AddNumberToObject(root, "timestamp", msg.timestamp_operacion);

    char* json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    // Publicar telemetría
    char topic[64];
    snprintf(topic, sizeof(topic), "domotica/nodos/%s/status", mac_str);
    
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, json_str, 0, 1, 0);
    free(json_str);

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Fallo al publicar telemetría en MQTT (msg_id: %d)", msg_id);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Telemetría publicada en MQTT de forma exitosa (Topic: %s)", topic);
    return ESP_OK;
}

} // namespace Gateway::Mqtt
