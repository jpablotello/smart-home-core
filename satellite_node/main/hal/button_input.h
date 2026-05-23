#pragma once

#include "periferico_base.h"
#include "driver/gpio.h"

class ButtonInput : public IEntradaDigital {
public:
    ButtonInput(gpio_num_t pin, bool active_low);
    ~ButtonInput() override = default;

    bool inicializar() override;
    bool estaActiva() const override;

private:
    gpio_num_t m_pin;
    bool       m_active_low;
    bool       m_inicializado;
};
