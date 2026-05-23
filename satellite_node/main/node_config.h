#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

namespace Node::Config {

constexpr gpio_num_t RGB_LED_PIN = GPIO_NUM_48;
constexpr gpio_num_t BUTTON_PIN = GPIO_NUM_0;
constexpr bool BUTTON_ACTIVE_LOW = true;

constexpr gpio_num_t USB_NATIVE_DM_PIN = GPIO_NUM_19;
constexpr gpio_num_t USB_NATIVE_DP_PIN = GPIO_NUM_18;
constexpr gpio_num_t UART0_TX_PIN = GPIO_NUM_43;
constexpr gpio_num_t UART0_RX_PIN = GPIO_NUM_44;

constexpr spi_host_device_t SPI_HOST = SPI2_HOST;
constexpr gpio_num_t SPI_MOSI_PIN = GPIO_NUM_11;
constexpr gpio_num_t SPI_MISO_PIN = GPIO_NUM_13;
constexpr gpio_num_t SPI_SCLK_PIN = GPIO_NUM_12;
constexpr gpio_num_t SPI_CS_PIN = GPIO_NUM_10;

} // namespace Node::Config
