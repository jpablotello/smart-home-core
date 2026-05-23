#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

namespace Node::Config {

// WAVESHARE ESP32-C6-ZERO pinout adaptation
// Onboard WS2812 RGB LED -> IO8
constexpr gpio_num_t RGB_LED_PIN = GPIO_NUM_8;
// Onboard BOOT button -> IO9 (active low)
constexpr gpio_num_t BUTTON_PIN = GPIO_NUM_9;
constexpr bool BUTTON_ACTIVE_LOW = true;

// Native USB (D-/D+)
constexpr gpio_num_t USB_NATIVE_DM_PIN = GPIO_NUM_14; // IO14
constexpr gpio_num_t USB_NATIVE_DP_PIN = GPIO_NUM_15; // IO15

// UART0 (debug): TX = IO23, RX = IO22
constexpr gpio_num_t UART0_TX_PIN = GPIO_NUM_23;
constexpr gpio_num_t UART0_RX_PIN = GPIO_NUM_22;

// SPI default mapping for this board (can be remapped)
constexpr spi_host_device_t SPI_HOST = SPI2_HOST;
constexpr gpio_num_t SPI_MOSI_PIN = GPIO_NUM_6;  // IO6
constexpr gpio_num_t SPI_MISO_PIN = GPIO_NUM_7;  // IO7
constexpr gpio_num_t SPI_SCLK_PIN = GPIO_NUM_5;  // IO5
constexpr gpio_num_t SPI_CS_PIN   = GPIO_NUM_4;  // IO4

} // namespace Node::Config
