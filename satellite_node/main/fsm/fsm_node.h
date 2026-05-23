// fsm_node.h
#pragma once

#include "periferico_base.h"
#include "domo_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum class FsmState {
    STATE_INIT,
    STATE_IDLE,
    STATE_READ_PERIPHERALS,
    STATE_EXECUTE_CMD,
    STATE_TRANSMIT,
    STATE_ERROR_RECOVERY
};

class FsmNode {
public:
    /**
     * @brief Constructor con inyección de dependencias para periféricos.
     * 
     * @param actuador Actuador controlado por comandos remotos.
     * @param boton Entrada digital reportada en telemetría.
     * @param sensor_spi Sensor numérico reportado en telemetría.
     */
    FsmNode(IActuador* actuador, IEntradaDigital* boton, ISensorNumerico* sensor_spi);
    ~FsmNode() = default;

    /**
     * @brief Inicia el hilo de la máquina de estados.
     */
    void iniciar();

private:
    // Hilo/Tarea interna de FreeRTOS
    static void tareaFsm(void* pvParameters);
    void ejecutarFsm();

    // Métodos para manejar cada estado
    void manejarEstadoInit();
    void manejarEstadoIdle();
    void manejarEstadoReadPeripherals();
    void manejarEstadoExecuteCmd();
    void manejarEstadoTransmit();
    void manejarEstadoErrorRecovery();

    // Variables internas
    FsmState        m_estado;
    IActuador*       m_actuador;
    IEntradaDigital* m_boton;
    ISensorNumerico* m_sensor_spi;
    
    // Datos y Telemetría local
    float           m_temperatura_actual;
    float           m_humedad_actual;
    uint32_t        m_fallas_seguidas;
    
    // Comando actual recibido para ejecutar
    DomoMessage_t   m_comando_pendiente;
    bool            m_hay_comando_pendiente;
    uint8_t         m_button_pressed;
    uint8_t         m_spi_value;
};
