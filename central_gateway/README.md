# Central Gateway (NodeMCU ESP32-S3 N16R8) - README

## Overview

Central Gateway acts as the hub between satellite nodes (ESP-NOW) and an MQTT broker (Internet). It is configured for the NodeMCU ESP32-S3 N16R8 board and bridges short-range ESP-NOW messages to remote MQTT clients.

## ¿Cómo funciona? (resumen rápido)

- **Conexiones:** El *gateway* se conecta a Internet usando Wi‑Fi en modo Station (STA) y mantiene una conexión con un broker MQTT. Los *nodos satélite* se comunican exclusivamente con el gateway mediante **ESP‑NOW** (radio de corto alcance).
- **Flujo de datos:** La telemetría que envían los satélites llega primero al gateway por ESP‑NOW; el gateway la procesa y la publica en el broker MQTT. Los comandos provenientes del broker MQTT llegan al gateway y éste los reenvía por ESP‑NOW al satélite destino.
- **¿Todo pasa por el gateway?** Sí: en la arquitectura por defecto el gateway actúa como puente entre la red local de ESP‑NOW y la red IP/MQTT. Los satélites no acceden a Internet directamente; todas las comunicaciones nube↔satélite pasan por el gateway. Opcionalmente se pueden tener múltiples gateways coordinados vía MQTT.


## Board profile

- Target: `esp32s3`
- Flash: 16 MB
- PSRAM: 8 MB OSPI enabled
- Integrated WS2812 RGB LED: GPIO48, reserved for future gateway status indication
- BOOT button: GPIO0, active low
- Native USB: GPIO19/GPIO18 reserved for future USB provisioning
- UART0 debug: GPIO43/GPIO44
- Do not use GPIO35, GPIO36, GPIO37, GPIO26-GPIO32, or GPIO46 as an output.

## Architecture

- Wi‑Fi (Station mode) for Internet connectivity and MQTT
- ESP‑NOW for short-range communication with satellite nodes
- FreeRTOS tasks for processing and forwarding messages
- NVS for persistent configuration

High-level flow: startup -> NVS init -> event loop -> Wi‑Fi init -> ESP‑NOW init -> MQTT init -> process loop

## Where to update credentials

Change Wi‑Fi credentials in `central_gateway/main/gateway_config.h`:

```cpp
constexpr const char* WIFI_SSID = "YOUR_WIFI_SSID";
constexpr const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
constexpr const char* MQTT_BROKER_URI = "mqtt://broker.hivemq.com";
```

Files to edit:
- `central_gateway/main/gateway_config.h` — set `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER_URI`
- `central_gateway/main/gateway_wifi.cpp` / `.h` — Wi‑Fi internals and `is_connected()`
- `central_gateway/main/gateway_mqtt.cpp` / `.h` — MQTT initialization and publish logic

## Build and flash

Before building, activate the ESP-IDF environment from the shell:

```bash
source /Users/juantello/esp/esp-idf/export.sh
```

Then run the build commands from `central_gateway`:

```bash
idf.py fullclean
idf.py build
idf.py -p <PORT> flash monitor
```

Replace `<PORT>` with the UART USB-C serial device. The native USB pins are intentionally unused so they remain available for future first-boot provisioning.

If you prefer, run the commands from the workspace root as long as the current working directory is inside the ESP-IDF project tree.

## Adding new functionality (checklist)

1. If message payload requires new fields, update `DomoMessage_t` in `components/domo_protocol/include/domo_protocol.h`.
2. Update `gateway_espnow.cpp` — `espnow_processor_task` to parse new fields and route to MQTT.
3. Update `gateway_mqtt.cpp` to publish new telemetry or subscribe to new command topics.
4. Add/modify source files in `central_gateway/main/CMakeLists.txt` if needed.
5. Build and validate with serial logs and MQTT client.

## Example: verify MQTT and ESP‑NOW

For a complete step-by-step test flow, see:

- [TESTING.md](TESTING.md)
- [TESTING.html](TESTING.html)

Subscribe to telemetry using Mosquitto (or other client):

```bash
mosquitto_sub -h broker.hivemq.com -t 'domotica/nodos/+/status' -v
```

Published telemetry includes fields such as `button_pressed`, `spi_value`, `led_brightness`, `estado_solicitado`, `temperatura`, and `humedad`.
The gateway now uses the real ESP-IDF `mqtt` and `json` components, so this path is functional rather than a local mock.

Publish a command to the gateway (example JSON):

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":48,"estado":1}'
```

Use `estado: 1` to turn the satellite RGB LED on and `estado: 0` to turn it off. For reference, the gateway forwards the command over ESP-NOW to the satellite with the same `pin` and `estado`.
Use the `mac_origen` value from telemetry as the `mac` target for commands.

## Relevant files

- `central_gateway/main/main.cpp`
- `central_gateway/main/gateway_config.h`
- `central_gateway/main/board_config.h`
- `central_gateway/main/gateway_wifi.cpp` / `.h`
- `central_gateway/main/gateway_espnow.cpp` / `.h`
- `central_gateway/main/gateway_mqtt.cpp` / `.h`
- `components/domo_protocol/include/domo_protocol.h`

## Notes for production

- Replace public test broker with a secure/authorized broker (TLS, auth).
- Remove or secure any hardcoded credentials.
- Validate `sizeof(DomoMessage_t)` stays below ESP‑NOW MTU (~250 bytes).

## Preguntas frecuentes

- **¿Cómo funciona la comunicación entre dispositivos?**
	- Los *satélites* (nodos) se comunican con el *gateway* mediante ESP‑NOW (radio de corto alcance). El gateway actúa como puente entre la red ESP‑NOW y la red IP/MQTT.
	- El gateway se conecta a Internet via Wi‑Fi (Station mode) y mantiene conexión con un broker MQTT; publica telemetría recibida por ESP‑NOW en `domotica/nodos/<mac>/status`.
	- Los comandos remotos se publican en `domotica/gateway/cmd`; el gateway los recibe desde el broker y los reenvía por ESP‑NOW al satélite destino.

- **¿Los satélites acceden a Internet directamente?**
	- No por defecto: los satélites no tienen conectividad IP y dependen del gateway para todo el tráfico nube↔nodo. Para acceso directo habría que añadir Wi‑Fi/IP al firmware del satélite.

- **¿Todo pasa por el gateway?**
	- Sí: en la configuración estándar todo el tráfico entre la nube (MQTT) y los satélites pasa por el gateway. Es posible tener múltiples gateways que cooperen vía MQTT si lo necesitas.

