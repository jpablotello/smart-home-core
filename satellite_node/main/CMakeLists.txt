# CMakeLists.txt para el componente main del Nodo Satélite
idf_component_register(SRCS "main.cpp"
                            "node_espnow.cpp"
                            "hal/actuador_rele.cpp"
                            "fsm/fsm_node.cpp"
                       INCLUDE_DIRS "." "hal" "fsm"
                       REQUIRES nvs_flash esp_wifi esp_event esp_now driver)
