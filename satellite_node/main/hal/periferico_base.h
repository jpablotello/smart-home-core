// periferico_base.h
#pragma once

#include "domo_protocol.h"

class PerifericoBase {
public:
    virtual ~PerifericoBase() = default;

    /**
     * @brief Configura e inicializa el periférico físico.
     * 
     * @return true Si la inicialización fue exitosa.
     * @return false Si ocurrió algún error.
     */
    virtual bool inicializar() = 0;

    /**
     * @brief Ejecuta una acción física en el actuador (encender, apagar, etc.).
     * 
     * @param accion Estado solicitado.
     */
    virtual void ejecutarAccion(EstadoAccion accion) = 0;

    /**
     * @brief Lee un valor del sensor (por ejemplo, sensor de temperatura/humedad).
     * 
     * @return float Valor medido (0.0f por defecto para actuadores puros).
     */
    virtual float leerDato() { 
        return 0.0f; 
    }
};
