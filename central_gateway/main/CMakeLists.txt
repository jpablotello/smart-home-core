# CMakeLists.txt para el componente main del Gateway Central
idf_component_register(SRCS "main.cpp"
                            "gateway_wifi.cpp"
                            "gateway_mqtt.cpp"
                            "gateway_espnow.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash esp_wifi esp_event domo_protocol)
