#pragma once

/**
 * @brief Mock MQTT Client Header
 * This is a minimal mock implementation to allow compilation without esp-mqtt component.
 * For production use, install esp-mqtt component: idf.py add-dependency esp-mqtt
 */

#include "esp_err.h"
#include "esp_event.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type definitions (simplified mocks)
typedef void* esp_mqtt_client_handle_t;

typedef enum {
    MQTT_EVENT_ANY = -1,
    MQTT_EVENT_ERROR = 0,
    MQTT_EVENT_CONNECTED,
    MQTT_EVENT_DISCONNECTED,
    MQTT_EVENT_SUBSCRIBED,
    MQTT_EVENT_UNSUBSCRIBED,
    MQTT_EVENT_PUBLISHED,
    MQTT_EVENT_DATA,
    MQTT_EVENT_BEFORE_CONNECT,
    MQTT_EVENT_DELETED,
} esp_mqtt_event_id_t;

typedef struct {
    int32_t msg_id;
    int32_t topic_len;
    int32_t data_len;
    const char* topic;
    const char* data;
    esp_mqtt_client_handle_t client;
} esp_mqtt_event_t;

typedef esp_mqtt_event_t* esp_mqtt_event_handle_t;

typedef struct {
    struct {
        struct {
            const char* uri;
        } address;
    } broker;
} esp_mqtt_client_config_t;

// Stub function declarations
static inline esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config) {
    return nullptr;  // Not implemented in mock
}

static inline esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client) {
    return ESP_OK;  // Stubbed out
}

static inline esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client,
                                                        esp_mqtt_event_id_t event_id,
                                                        esp_event_handler_t event_handler,
                                                        void* event_handler_arg) {
    return ESP_OK;  // Stubbed out
}

static inline int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client,
                                             const char *topic, int qos) {
    return -1;  // Stubbed out
}

static inline int esp_mqtt_client_publish(esp_mqtt_client_handle_t client,
                                           const char *topic, const char *data, int len,
                                           int qos, int retain) {
    return -1;  // Stubbed out
}

#ifdef __cplusplus
}
#endif
