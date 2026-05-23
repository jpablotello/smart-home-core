#pragma once

#include "periferico_base.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <stdint.h>

class SpiSensor : public ISensorNumerico {
public:
    SpiSensor(spi_host_device_t host,
              gpio_num_t mosi_pin,
              gpio_num_t miso_pin,
              gpio_num_t sclk_pin,
              gpio_num_t cs_pin);
    ~SpiSensor() override = default;

    bool inicializar() override;
    uint8_t leerValor() override;

private:
    spi_host_device_t   m_host;
    gpio_num_t          m_mosi_pin;
    gpio_num_t          m_miso_pin;
    gpio_num_t          m_sclk_pin;
    gpio_num_t          m_cs_pin;
    spi_device_handle_t m_spi;
};
