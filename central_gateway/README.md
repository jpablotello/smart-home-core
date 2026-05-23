# Central Gateway (ESP32) - README

## Overview

Central Gateway acts as the hub between satellite nodes (ESP-NOW) and an MQTT broker (Internet). It runs on an ESP32 and bridges short-range ESP-NOW messages to remote MQTT clients.

## Architecture

- Wi‑Fi (Station mode) for Internet connectivity and MQTT
- ESP‑NOW for short-range communication with satellite nodes
- FreeRTOS tasks for processing and forwarding messages
- NVS for persistent configuration

High-level flow: startup -> NVS init -> event loop -> Wi‑Fi init -> ESP‑NOW init -> MQTT init -> process loop

## Where to update credentials

Change Wi‑Fi credentials in `central_gateway/main/main.cpp` near the top of `app_main`:

```cpp
const char* wifi_ssid = "YOUR_WIFI_SSID";
const char* wifi_pass = "YOUR_WIFI_PASSWORD";
// later:
const char* mqtt_broker = "mqtt://broker.hivemq.com"; // change to your broker URI
```

Files to edit:
- `central_gateway/main/main.cpp` — set `wifi_ssid`, `wifi_pass`, `mqtt_broker`
- `central_gateway/main/gateway_wifi.cpp` / `.h` — Wi‑Fi internals and `is_connected()`
- `central_gateway/main/gateway_mqtt.cpp` / `.h` — MQTT initialization and publish logic

## Build and flash

Use ESP-IDF tools from the project root (`central_gateway` is an IDF component inside the workspace). Typical commands:

```bash
# from workspace or central_gateway folder
idf.py fullclean
idf.py build
idf.py -p <PORT> flash monitor
```

Replace `<PORT>` with your serial device (macOS example: `/dev/tty.SLAB_USBtoUART`).

## Adding new functionality (checklist)

1. If message payload requires new fields, update `DomoMessage_t` in `components/domo_protocol/include/domo_protocol.h`.
2. Update `gateway_espnow.cpp` — `espnow_processor_task` to parse new fields and route to MQTT.
3. Update `gateway_mqtt.cpp` to publish new telemetry or subscribe to new command topics.
4. Add/modify source files in `central_gateway/main/CMakeLists.txt` if needed.
5. Build and validate with serial logs and MQTT client.

## Example: verify MQTT and ESP‑NOW

Subscribe to telemetry using Mosquitto (or other client):

```bash
mosquitto_sub -h broker.hivemq.com -t 'domotica/nodos/+/status' -v
```

Publish a command to the gateway (example JSON):

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":2,"estado":1}'
```

## Relevant files

- `central_gateway/main/main.cpp`
- `central_gateway/main/gateway_wifi.cpp` / `.h`
- `central_gateway/main/gateway_espnow.cpp` / `.h`
- `central_gateway/main/gateway_mqtt.cpp` / `.h`
- `components/domo_protocol/include/domo_protocol.h`

## Notes for production

- Replace public test broker with a secure/authorized broker (TLS, auth).
- Remove or secure any hardcoded credentials.
- Validate `sizeof(DomoMessage_t)` stays below ESP‑NOW MTU (~250 bytes).
