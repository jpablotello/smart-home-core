# Central Gateway - Step-by-step test guide

This guide explains how to test the Central Gateway from zero: configure Wi-Fi, flash the board, verify MQTT, receive satellite telemetry, and send commands from the network to turn the satellite RGB LED on/off.

## 1. What you need

- One NodeMCU ESP32-S3 N16R8 running `central_gateway`.
- One satellite node running `satellite_node`.
- Both boards powered and close enough for ESP-NOW.
- A 2.4 GHz Wi-Fi network. ESP32 does not connect to 5 GHz-only networks.
- An MQTT client. Examples below use `mosquitto_sub` and `mosquitto_pub`.

Default MQTT settings:

```text
Broker: mqtt://broker.hivemq.com
Command topic: domotica/gateway/cmd
Telemetry topic: domotica/nodos/<satellite_mac>/status
```

## 2. Configure the gateway

Edit `central_gateway/main/gateway_config.h`:

```cpp
constexpr const char* WIFI_SSID = "YOUR_WIFI_SSID";
constexpr const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
constexpr const char* MQTT_BROKER_URI = "mqtt://broker.hivemq.com";
```

For a first test, keep the public HiveMQ broker. For a real installation, use your own broker with authentication/TLS.

## 3. Build and flash the Central Gateway

From the workspace:

```bash
cd /Users/juantello/Works/Esp32/Domotica/central_gateway
source /Users/juantello/esp/esp-idf/export.sh
idf.py build
```

Connect the UART USB-C port of the NodeMCU ESP32-S3 and find the serial port:

```bash
ls /dev/cu.*
```

Flash and open the monitor:

```bash
idf.py -p /dev/cu.<YOUR_PORT> flash monitor
```

Expected monitor logs:

```text
Iniciando Gateway Central de Domótica...
Configurando Wi-Fi...
Configurando protocolo ESP-NOW...
Configurando MQTT broker...
Conectado al broker MQTT.
Suscrito a: domotica/gateway/cmd
Gateway Central inicializado de manera exitosa.
```

## 4. Build and flash the satellite

In another terminal:

```bash
cd /Users/juantello/Works/Esp32/Domotica/satellite_node
source /Users/juantello/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.<SATELLITE_PORT> flash monitor
```

The satellite uses the integrated WS2812 RGB LED on GPIO48, so you do not need external wiring to test the LED command path.

## 5. Watch telemetry from the network

Open a terminal on your computer and subscribe to all node telemetry:

```bash
mosquitto_sub -h broker.hivemq.com -t 'domotica/nodos/+/status' -v
```

When the satellite sends data and the gateway republishes it, you should see something like:

```json
domotica/nodos/aa:bb:cc:dd:ee:ff/status {"mac_origen":"aa:bb:cc:dd:ee:ff","tipo_nodo":4,"pin_afectado":48,"estado_solicitado":0,"led_brightness":0,"button_pressed":0,"spi_value":23,"temperatura":25.5,"humedad":60.0,"timestamp":12345}
```

Copy the value of `mac_origen`. You will use it as the target MAC for commands.

## 6. Turn the satellite RGB LED on

Replace `aa:bb:cc:dd:ee:ff` with the `mac_origen` seen in telemetry:

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":48,"estado":1}'
```

Expected result:

- Gateway monitor logs `Procesando comando recibido por MQTT`.
- Gateway logs `Ruteando comando por ESP-NOW`.
- Satellite receives the command.
- Satellite integrated RGB LED turns green.

## 7. Turn the satellite RGB LED off

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":48,"estado":0}'
```

Expected result:

- Satellite integrated RGB LED turns off.
- Next telemetry should report `led_brightness: 0` and `estado_solicitado: 0`.

## 8. Read the satellite button from the network

The satellite reads the integrated BOOT button on GPIO0 as active low.

Keep the telemetry subscriber running:

```bash
mosquitto_sub -h broker.hivemq.com -t 'domotica/nodos/+/status' -v
```

Press and hold BOOT on the satellite until a telemetry frame arrives. In the JSON:

```json
"button_pressed":1
```

When released:

```json
"button_pressed":0
```

## 9. Read the satellite SPI value from the network

The satellite SPI test bus is configured as:

```text
MOSI: GPIO11
MISO: GPIO13
SCLK: GPIO12
CS:   GPIO10
```

The gateway republishes the satellite SPI reading as:

```json
"spi_value":23
```

If no SPI device is connected, the value can be meaningless or remain at a fallback value. The communication path is still testable because the field appears in MQTT telemetry.

## 10. MQTT command format

The gateway only processes JSON messages published to:

```text
domotica/gateway/cmd
```

Required JSON fields:

```json
{
  "mac": "aa:bb:cc:dd:ee:ff",
  "pin": 48,
  "estado": 1
}
```

Field meaning:

- `mac`: satellite MAC from telemetry `mac_origen`.
- `pin`: currently informational for the satellite RGB test; use `48`.
- `estado`: `1` turns on the RGB LED, `0` turns it off, `99` maps to failure/red if sent through the actuator path.

## 11. Troubleshooting

- No `Conectado al broker MQTT`: check Wi-Fi SSID/password and that the network is 2.4 GHz.
- No MQTT telemetry: confirm the gateway monitor receives ESP-NOW frames from the satellite.
- Gateway receives telemetry but MQTT client sees nothing: confirm broker host and topic `domotica/nodos/+/status`.
- Commands do nothing: copy the exact `mac_origen` from telemetry and publish to `domotica/gateway/cmd`.
- ESP-NOW unreliable: keep both boards near each other and on the same Wi-Fi channel. The gateway uses Wi-Fi station mode, so its ESP-NOW channel follows the connected AP.
- Public broker noise: HiveMQ is shared. Use unique topics or a private broker if other clients interfere.

## 12. Quick command summary

Terminal A, gateway:

```bash
cd /Users/juantello/Works/Esp32/Domotica/central_gateway
source /Users/juantello/esp/esp-idf/export.sh
idf.py -p /dev/cu.<GATEWAY_PORT> flash monitor
```

Terminal B, satellite:

```bash
cd /Users/juantello/Works/Esp32/Domotica/satellite_node
source /Users/juantello/esp/esp-idf/export.sh
idf.py -p /dev/cu.<SATELLITE_PORT> flash monitor
```

Terminal C, telemetry:

```bash
mosquitto_sub -h broker.hivemq.com -t 'domotica/nodos/+/status' -v
```

Terminal D, commands:

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":48,"estado":1}'
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":48,"estado":0}'
```
