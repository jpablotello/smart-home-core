# Satellite Node — WAVESHARE ESP32-C6-ZERO

## Overview

Este repositorio contiene el firmware del Nodo Satélite. Se comunica con la Central Gateway mediante ESP-NOW para recibir comandos y enviar telemetría; la Central Gateway reexpone la telemetría y recibe comandos vía MQTT.

## Board profile — WAVESHARE ESP32-C6-ZERO

- MCU: ESP32-C6FH4 (RISC‑V)
- Wireless: Wi‑Fi 6, Bluetooth 5, IEEE 802.15.4
- Onboard RGB (WS2812): IO8 -> `RGB_LED_PIN = GPIO_NUM_8`
- BOOT button: IO9 -> `BUTTON_PIN = GPIO_NUM_9` (active low)
- UART0 (debug): TX = IO23, RX = IO22
- I2C default: SDA = IO2, SCL = IO3
- SPI default: CS=IO4, CLK=IO5, MOSI=IO6, MISO=IO7
- ADC channels: IO0, IO1, IO2, IO3, IO11

> Nota: El proyecto usa `Node::Config` en `node_config.h` para definir estos pines. Si necesitas remapear periféricos, actualiza ese archivo.

## MQTT + Comandos para encender/apagar el LED

El Gateway se suscribe al tópico de comandos definido en `central_gateway/main/gateway_config.h`:

- `domotica/gateway/cmd`

Para encender el LED integrado del `satellite_node` publica un JSON con los campos `mac`, `pin` y `estado`. Ejemplos usando `mosquitto_pub` contra `broker.hivemq.com`:

- Encender (LED en IO8):

```bash
mosquitto_pub -h broker.hivemq.com -t "domotica/gateway/cmd" -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":8,"estado":1}'
```

- Apagar (LED en IO8):

```bash
mosquitto_pub -h broker.hivemq.com -t "domotica/gateway/cmd" -m '{"mac":"aa:bb:cc:dd:ee:ff","pin":8,"estado":0}'
```

Reemplaza `aa:bb:cc:dd:ee:ff` por la dirección MAC del nodo satélite (en minúsculas, formato hex separado por `:`). El Gateway reenviará el comando por ESP-NOW y el nodo responderá con un reporte que la Gateway publicará en MQTT.

## Ver telemetría / Health check

Suscríbete a todos los tópicos de telemetría para observar las respuestas:

```bash
mosquitto_sub -h broker.hivemq.com -t 'domotica/nodos/+/status' -v
```

Health‑check rápido:

1. Ejecuta la suscripción anterior en una terminal.
2. En otra terminal publica el comando `estado=1` al `mac` del nodo con `mosquitto_pub` (ejemplos arriba).
3. Deberías ver en la suscripción un JSON publicado en `domotica/nodos/<mac>/status` indicando `pin_afectado`, `estado_solicitado`, `led_brightness`, etc. Si llega, gateway y nodo están comunicando correctamente.

Ejemplo de línea esperada (format ilustrativo):

```
domotica/nodos/aa:bb:cc:dd:ee:ff/status {"mac_origen":"aa:bb:cc:dd:ee:ff","tipo_nodo":4,"pin_afectado":8,"estado_solicitado":1,...}
```

## Build y flash

Activa el entorno ESP‑IDF y compila desde la carpeta `satellite_node`:

```bash
source /Users/juantello/esp/esp-idf/export.sh
idf.py fullclean
idf.py build
idf.py -p <PORT> flash monitor
```

## Archivos relevantes

- `satellite_node/main/main.cpp`
- `satellite_node/main/node_config.h` (mapeo de pines)
- `satellite_node/main/hal/led_rgb_ws2812.*` (driver WS2812)
- `satellite_node/main/fsm/fsm_node.*` (lógica de ejecución de comandos y reportes)
- `components/domo_protocol/include/domo_protocol.h` (formato de mensaje)

## Notas

- El pin enviado en el JSON (`pin`) es un número GPIO; para el LED integrado usa `8`.
- Si no conoces la MAC del nodo, consulta los logs serie del dispositivo al arrancar (imprime la MAC local) o revisa la tabla de clientes del AP.
- Mantén `DomoMessage_t` pequeño por las limitaciones de ESP‑NOW (~250 bytes).
