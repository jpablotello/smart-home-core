 # Satellite Node (ESP32) - README

## Overview

Satellite Node is a modular ESP32 device that communicates with the Central Gateway using ESP‑NOW. It runs a finite state machine (`FsmNode`) which reads peripherals (sensors/actuators), executes incoming commands, and reports telemetry.

## Architecture

- `FsmNode` manages states: INIT, IDLE, READ_PERIPHERALS, EXECUTE_CMD, TRANSMIT, ERROR_RECOVERY
- `node_espnow` handles ESP‑NOW init, receive/send callbacks and an RX queue
- Peripherals are abstracted via `PerifericoBase` (e.g., `ActuadorRele`)

## How to add a sensor (analog) step-by-step

 
1. Update `DomoMessage_t` if you need to send additional fields: `components/domo_protocol/include/domo_protocol.h`.
2. Implement a peripheral driver (example sketch):

```cpp
// hal/sensor_temp.cpp (sketch)
class SensorTemp : public PerifericoBase {
public:
  bool inicializar() override { /* init ADC */ return true; }
  float leer() { /* read ADC and convert */ return 0.0f; }
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
int state = gpio_get_level(GPIO_NUM_0); // adjust pin
if (state == 1) {
  // mark event and include in DomoMessage_t
}
```

## Build and flash

```bash
idf.py fullclean
idf.py build
idf.py -p <PORT> flash monitor
```

## Testing and verification

1. With the gateway running, verify the node registers the gateway MAC and successfully sends telemetry (check serial logs).
2. Use `mosquitto_sub` on the gateway side to confirm telemetry reaches MQTT.

## Relevant files

- `satellite_node/main/main.cpp`
- `satellite_node/main/node_espnow.cpp` / `.h`
- `satellite_node/main/fsm/fsm_node.cpp` / `.h`
- `satellite_node/main/hal/*` — peripheral drivers
- `components/domo_protocol/include/domo_protocol.h`

## Notes

- Keep `DomoMessage_t` small due to ESP‑NOW payload size limits (~250 bytes).
- Use `esp_now_add_peer` on the node to ensure the gateway is a peer before sending.
