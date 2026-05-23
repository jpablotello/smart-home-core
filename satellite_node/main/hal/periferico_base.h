#pragma once

#include "domo_protocol.h"
#include <stdint.h>

class Inicializable {
public:
    virtual ~Inicializable() = default;

    virtual bool inicializar() = 0;
};

class IActuador : public Inicializable {
public:
    ~IActuador() override = default;

    virtual void ejecutarAccion(EstadoAccion accion) = 0;
    virtual EstadoAccion obtenerEstado() const = 0;
};

class IEntradaDigital : public Inicializable {
public:
    ~IEntradaDigital() override = default;

    virtual bool estaActiva() const = 0;
};

class ISensorNumerico : public Inicializable {
public:
    ~ISensorNumerico() override = default;

    virtual uint8_t leerValor() = 0;
};
