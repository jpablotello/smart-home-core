Comandos MQTT listos para copiar
================================

Copiar y pegar tal cual (reemplaza <MAC> por la MAC del satélite, por ejemplo aa:bb:cc:dd:ee:ff).

1) Ver telemetría (suscribirse a todos los satélites):

```bash
mosquitto_sub -h broker.hivemq.com -t 'domotica/nodos/+/status' -v
```

2) Encender LED del satélite (ejemplo):

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"<MAC>","pin":48,"estado":1}'
```

3) Apagar LED del satélite (ejemplo):

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"<MAC>","pin":48,"estado":0}'
```

4) Enviar estado de falla/rojo (ejemplo):

```bash
mosquitto_pub -h broker.hivemq.com -t 'domotica/gateway/cmd' -m '{"mac":"<MAC>","pin":48,"estado":99}'
```

5) Ejemplo JSON (formato legible):

```json
{
  "mac": "aa:bb:cc:dd:ee:ff",
  "pin": 48,
  "estado": 1
}
```

6) Notas rápidas
- Copia la `mac_origen` desde la telemetría publicada en `domotica/nodos/<mac>/status`.
- El `pin` es informativo para la prueba integrada (usar 48 para el WS2812).
- El tópico de comando usado por el gateway es `domotica/gateway/cmd`.

---

Archivo: central_gateway/COMMANDS.md
