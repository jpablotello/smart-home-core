# CMakeLists.txt para el componente main del Nodo Satélite
idf_component_register(SRCS "main.cpp"
                            "node_espnow.cpp"
                            "hal/actuador_rele.cpp"
                            "hal/button_input.cpp"
                            "hal/led_rgb_ws2812.cpp"
                            "hal/spi_sensor.cpp"
                            "fsm/fsm_node.cpp"
                       INCLUDE_DIRS "." "hal" "fsm"
                       REQUIRES nvs_flash esp_wifi esp_event driver led_strip domo_protocol)
