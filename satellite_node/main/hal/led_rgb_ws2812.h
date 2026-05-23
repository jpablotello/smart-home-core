#pragma once

#include "periferico_base.h"
#include "driver/gpio.h"
#include "led_strip.h"

class LedRgbWs2812 : public IActuador {
public:
    explicit LedRgbWs2812(gpio_num_t pin);
    ~LedRgbWs2812() override = default;

    bool inicializar() override;
    void ejecutarAccion(EstadoAccion accion) override;
    EstadoAccion obtenerEstado() const override { return m_estado_actual; }

    void setColor(uint8_t red, uint8_t green, uint8_t blue);

private:
    gpio_num_t          m_pin;
    led_strip_handle_t  m_strip;
    EstadoAccion        m_estado_actual;
};
