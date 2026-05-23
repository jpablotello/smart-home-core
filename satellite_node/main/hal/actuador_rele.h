// actuador_rele.h
#pragma once

#include "periferico_base.h"
#include "driver/gpio.h"

class ActuadorRele : public IActuador {
public:
    /**
     * @brief Constructor para el actuador de relé.
     * 
     * @param pin Número del pin GPIO a controlar (ej: GPIO_NUM_2).
     */
    explicit ActuadorRele(gpio_num_t pin);
    ~ActuadorRele() override = default;

    bool inicializar() override;
    void ejecutarAccion(EstadoAccion accion) override;
    
    EstadoAccion obtenerEstado() const override { return m_estado_actual; }

private:
    gpio_num_t   m_pin;
    EstadoAccion m_estado_actual;
};
