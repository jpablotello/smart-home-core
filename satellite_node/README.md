# Satellite Node (NodeMCU ESP32-S3 N16R8) - README

## Overview

Satellite Node is a modular ESP32-S3 device that communicates with the Central Gateway using ESP-NOW. It runs a finite state machine (`FsmNode`) which reads peripherals (sensors/actuators), executes incoming commands, and reports telemetry.

## Board profile and safe pinout

- Target: `esp32s3`
- Flash: 16 MB
- PSRAM: 8 MB OSPI enabled
- Integrated WS2812 RGB LED: GPIO48
- BOOT button: GPIO0, active low
- SPI sensor bus: MOSI GPIO11, MISO GPIO13, SCLK GPIO12, CS GPIO10
- Native USB: GPIO19/GPIO18 reserved for future USB provisioning
- UART0 debug: GPIO43/GPIO44
- Do not use GPIO35, GPIO36, GPIO37, GPIO26-GPIO32, or GPIO46 as an output.

## Architecture

- `FsmNode` manages states: INIT, IDLE, READ_PERIPHERALS, EXECUTE_CMD, TRANSMIT, ERROR_RECOVERY
- `node_espnow` handles ESP‑NOW init, receive/send callbacks and an RX queue
- Peripherals are injected through small interfaces (`IActuador`, `IEntradaDigital`, `ISensorNumerico`)
- `LedRgbWs2812` drives the integrated RGB LED and lets you test network commands without external wiring

## How to add a sensor (analog) step-by-step

 
1. Update `DomoMessage_t` if you need to send additional fields: `components/domo_protocol/include/domo_protocol.h`.
2. Implement a peripheral driver (example sketch):

```cpp
// hal/sensor_temp.cpp (sketch)
class SensorTemp : public ISensorNumerico {
public:
  bool inicializar() override { /* init ADC */ return true; }
  int leerValor() override { /* read ADC and convert */ return 0; }
};
```

3. Instantiate the peripheral and inject into `FsmNode` in `satellite_node/main/main.cpp` (see existing `ActuadorRele` instance).
4. In `fsm_node.cpp` add reading logic in `manejarEstadoReadPeripherals()` and populate `DomoMessage_t.lectura_temperatura` or new fields.
5. Use `Node::Espnow::send_report()` to send the `DomoMessage_t` to the gateway.

## How to add a button (digital input)

1. Choose a free GPIO pin and configure it as input in a new peripheral under `hal/`.
2. Add debounce logic in the peripheral or in the FSM.
3. On button press, set a flag or enqueue an event so the FSM transitions to `EXECUTE_CMD` or `TRANSMIT`.

Example (reading a button):

```cpp
int state = gpio_get_level(Node::Config::BUTTON_PIN);
if (state == 0) {
  // mark event and include in DomoMessage_t
}
```

This project already includes an example button input and SPI sensor implementation: the node reads `button_pressed` and `spi_value` and sends them in the telemetry message to the gateway. The gateway publishes these values to MQTT and you can monitor them from the network. If the node has not yet learned the gateway MAC, it sends telemetry using ESP-NOW broadcast; after receiving a command, it registers that gateway as a peer and sends direct reports.

## Build and flash

Before building, activate the ESP-IDF environment from the shell:

```bash
source /Users/juantello/esp/esp-idf/export.sh
```

Then run the build commands from `satellite_node`:

```bash
idf.py fullclean
idf.py build
idf.py -p <PORT> flash monitor
```

Replace `<PORT>` with the UART USB-C serial device. The native USB pins are intentionally unused so they remain available for future first-boot provisioning.

## Testing and verification

1. With the gateway running, verify the node sends telemetry by broadcast first and registers the gateway MAC after receiving a command (check serial logs).
2. Use `mosquitto_sub` on the gateway side to confirm telemetry reaches MQTT.
3. Publish a command with `estado: 1` to turn the integrated RGB LED green, and `estado: 0` to turn it off.

## Relevant files

- `satellite_node/main/main.cpp`
- `satellite_node/main/node_config.h`
- `satellite_node/main/node_espnow.cpp` / `.h`
- `satellite_node/main/fsm/fsm_node.cpp` / `.h`
- `satellite_node/main/hal/*` — peripheral drivers
- `components/domo_protocol/include/domo_protocol.h`

## Notes

- Keep `DomoMessage_t` small due to ESP‑NOW payload size limits (~250 bytes).
- Broadcast telemetry requires the satellite and gateway to be on the same Wi-Fi channel. If telemetry does not appear, check the gateway AP channel and configure the satellite Wi-Fi channel accordingly.
