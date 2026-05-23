#pragma once

#include "driver/gpio.h"

namespace Gateway::Board {

constexpr gpio_num_t RGB_LED_PIN = GPIO_NUM_48;
constexpr gpio_num_t BOOT_BUTTON_PIN = GPIO_NUM_0;
constexpr bool BOOT_BUTTON_ACTIVE_LOW = true;

constexpr gpio_num_t USB_NATIVE_DM_PIN = GPIO_NUM_19;
constexpr gpio_num_t USB_NATIVE_DP_PIN = GPIO_NUM_18;
constexpr gpio_num_t UART0_TX_PIN = GPIO_NUM_43;
constexpr gpio_num_t UART0_RX_PIN = GPIO_NUM_44;

} // namespace Gateway::Board
