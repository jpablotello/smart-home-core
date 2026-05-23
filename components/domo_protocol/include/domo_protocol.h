// domo_protocol.h
#pragma once
#include <stdint.h>

enum class TipoNodo : uint8_t { 
    SENSOR_AMBIENTE = 0x01, 
    ACTUADOR_DIGITAL = 0x02, 
    CONTROL_RIEGO = 0x03, 
    HIBRIDO = 0x04 
};

enum class EstadoAccion : uint8_t { 
    APAGADO = 0, 
    ENCENDIDO = 1, 
    FALLA = 99 
};

struct __attribute__((packed)) DomoMessage_t {
    uint8_t      mac_origen[6];
    TipoNodo     tipo_nodo;
    uint8_t      pin_afectado;
    uint8_t      estado_solicitado; // Utilizado también como estado actual en los reportes
    float        lectura_temperatura;
    float        lectura_humedad;
    uint32_t     timestamp_operacion;
};
