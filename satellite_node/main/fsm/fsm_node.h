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
     * @brief Constructor con inyección de dependencias para los periféricos.
     * 
     * @param actuador Puntero a un periférico que actúe como actuador (ej: ActuadorRele).
     */
    explicit FsmNode(PerifericoBase* actuador);
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
    PerifericoBase* m_actuador;
    
    // Datos y Telemetría local
    float           m_temperatura_actual;
    float           m_humedad_actual;
    uint32_t        m_fallas_seguidas;
    
    // Comando actual recibido para ejecutar
    DomoMessage_t   m_comando_pendiente;
    bool            m_hay_comando_pendiente;
};
