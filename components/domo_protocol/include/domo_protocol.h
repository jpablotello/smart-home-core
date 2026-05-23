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
    uint8_t      estado_solicitado; // Comando o estado actual del actuador
    uint8_t      button_pressed;    // 0 = no, 1 = yes
    uint8_t      spi_value;        // Valor leído por SPI (0..100)
    uint8_t      led_brightness;   // 0..100 brightness/estado del LED
    float        lectura_temperatura;
    float        lectura_humedad;
    uint32_t     timestamp_operacion;
};
