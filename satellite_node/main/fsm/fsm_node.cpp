// fsm_node.cpp
#include "fsm_node.h"
#include "node_espnow.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <stdlib.h>

static const char* TAG = "FSM_NODE";

FsmNode::FsmNode(PerifericoBase* actuador)
    : m_estado(FsmState::STATE_INIT),
      m_actuador(actuador),
      m_temperatura_actual(22.5f),
      m_humedad_actual(60.0f),
      m_fallas_seguidas(0),
      m_comando_pendiente({}),
      m_hay_comando_pendiente(false) {}

void FsmNode::iniciar() {
    BaseType_t err = xTaskCreate(tareaFsm, "fsm_task", 4096, this, 5, nullptr);
    if (err == pdPASS) {
        ESP_LOGI(TAG, "Tarea FSM creada con éxito.");
    } else {
        ESP_LOGE(TAG, "Fallo al crear la tarea FSM.");
    }
}

void FsmNode::tareaFsm(void* pvParameters) {
    auto self = static_cast<FsmNode*>(pvParameters);
    self->ejecutarFsm();
}

void FsmNode::ejecutarFsm() {
    ESP_LOGI(TAG, "Hilo FSM en marcha.");
    while (true) {
        switch (m_estado) {
            case FsmState::STATE_INIT:
                manejarEstadoInit();
                break;
            case FsmState::STATE_IDLE:
                manejarEstadoIdle();
                break;
            case FsmState::STATE_READ_PERIPHERALS:
                manejarEstadoReadPeripherals();
                break;
            case FsmState::STATE_EXECUTE_CMD:
                manejarEstadoExecuteCmd();
                break;
            case FsmState::STATE_TRANSMIT:
                manejarEstadoTransmit();
                break;
            case FsmState::STATE_ERROR_RECOVERY:
                manejarEstadoErrorRecovery();
                break;
        }
    }
}

void FsmNode::manejarEstadoInit() {
    ESP_LOGI(TAG, ">>> FSM: STATE_INIT <<<");

    // 1. Inicializar el actuador físico
    if (m_actuador != nullptr) {
        if (!m_actuador->inicializar()) {
            ESP_LOGE(TAG, "Fallo al inicializar el actuador.");
            m_estado = FsmState::STATE_ERROR_RECOVERY;
            return;
        }
    }

    // 2. Inicializar el stack de red local ESP-NOW
    esp_err_t err = Node::Espnow::init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar ESP-NOW en el nodo.");
        m_estado = FsmState::STATE_ERROR_RECOVERY;
        return;
    }

    ESP_LOGI(TAG, "Nodo inicializado correctamente. Transicionando a IDLE.");
    m_estado = FsmState::STATE_IDLE;
}

void FsmNode::manejarEstadoIdle() {
    ESP_LOGD(TAG, ">>> FSM: STATE_IDLE <<<");

    QueueHandle_t rx_queue = Node::Espnow::get_rx_queue();
    if (rx_queue == nullptr) {
        m_estado = FsmState::STATE_ERROR_RECOVERY;
        return;
    }

    // Bloqueo en la cola esperando comandos. Timeout de 10 segundos para reporte periódico
    DomoMessage_t msg;
    BaseType_t status = xQueueReceive(rx_queue, &msg, pdMS_TO_TICKS(10000));

    if (status == pdTRUE) {
        ESP_LOGI(TAG, "Comando recibido desde ESP-NOW. Procesando...");
        m_comando_pendiente = msg;
        m_hay_comando_pendiente = true;
        m_estado = FsmState::STATE_EXECUTE_CMD;
    } else {
        // Expiró el tiempo de espera sin comandos -> Ejecutar lectura periódica
        ESP_LOGI(TAG, "Timeout de espera. Iniciando lectura periódica de sensores...");
        m_estado = FsmState::STATE_READ_PERIPHERALS;
    }
}

void FsmNode::manejarEstadoReadPeripherals() {
    ESP_LOGI(TAG, ">>> FSM: STATE_READ_PERIPHERALS <<<");

    // Simulación de lectura de sensor de temperatura y humedad (con variación aleatoria)
    float delta_temp = (static_cast<float>(rand() % 10) - 5.0f) / 10.0f; // -0.5 a +0.5
    float delta_hum = (static_cast<float>(rand() % 20) - 10.0f) / 10.0f; // -1.0 a +1.0

    m_temperatura_actual += delta_temp;
    m_humedad_actual += delta_hum;

    // Limitar valores dentro de rangos normales
    if (m_temperatura_actual < 15.0f) m_temperatura_actual = 15.0f;
    if (m_temperatura_actual > 35.0f) m_temperatura_actual = 35.0f;
    if (m_humedad_actual < 30.0f) m_humedad_actual = 30.0f;
    if (m_humedad_actual > 90.0f) m_humedad_actual = 90.0f;

    ESP_LOGI(TAG, "Lectura de Sensores -> Temperatura: %.2f °C | Humedad: %.2f %%", 
             m_temperatura_actual, m_humedad_actual);

    // Transicionar para transmitir los datos
    m_estado = FsmState::STATE_TRANSMIT;
}

void FsmNode::manejarEstadoExecuteCmd() {
    ESP_LOGI(TAG, ">>> FSM: STATE_EXECUTE_CMD <<<");

    if (m_hay_comando_pendiente && m_actuador != nullptr) {
        // Mapear el estado solicitado (uint8_t) al enum EstadoAccion
        EstadoAccion accion = EstadoAccion::APAGADO;
        if (m_comando_pendiente.estado_solicitado == 1) {
            accion = EstadoAccion::ENCENDIDO;
        } else if (m_comando_pendiente.estado_solicitado == 99) {
            accion = EstadoAccion::FALLA;
        }

        m_actuador->ejecutarAccion(accion);
        m_hay_comando_pendiente = false;
        
        // Transicionar a TRANSMIT para reportar el nuevo estado de vuelta
        m_estado = FsmState::STATE_TRANSMIT;
    } else {
        m_estado = FsmState::STATE_IDLE;
    }
}

void FsmNode::manejarEstadoTransmit() {
    ESP_LOGI(TAG, ">>> FSM: STATE_TRANSMIT <<<");

    // Construir el reporte
    DomoMessage_t msg = {};
    
    // Obtener MAC local
    esp_wifi_get_mac(WIFI_IF_STA, msg.mac_origen);
    msg.tipo_nodo = TipoNodo::ACTUADOR_DIGITAL;
    msg.pin_afectado = 2; // Pin de control del relé de prueba
    
    // Reportar el estado actual del actuador (para propósitos del reporte, estado_solicitado = estado_actual)
    msg.estado_solicitado = 0;
    if (m_actuador != nullptr) {
        // Recuperamos el estado del actuador haciendo un cast seguro
        // o asumiendo el estado guardado en el actuador
        // Para simplificar, leemos el estado simulado/actual
        // Si es 1 = ENCENDIDO, 0 = APAGADO
        // Para el reporte de telemetría usamos el estado reflejado
        // En una implementación real, podemos tener un método en PerifericoBase
        // o una consulta directa. Aquí usamos el valor real de GPIO si fuera bidireccional.
        // Dado que m_actuador implementa obtenerEstado, podemos hacer un cast o guardar el estado localmente.
        // Para desacoplamiento, utilizaremos la acción de retorno.
        // Para no forzar casting, el actuador escribe su estado en el hardware, 
        // asumimos que el estado actual es mapeable.
    }
    
    // Asumiremos el último estado del actuador
    // El relé tiene una función obtenerEstado()
    // Hacemos un cast rápido y seguro
    // Si no es relé, por defecto es APAGADO (0)
    // En C++17, se puede utilizar dynamic_cast pero requiere RTTI. Usamos static_cast de forma segura
    // ya que sabemos que en app_main inyectamos un ActuadorRele.
    // De todos modos, para evitar problemas si RTTI está desactivado, guardamos un estado interno.
    // O leemos el nivel del GPIO directamente:
    int nivel_pin = gpio_get_level(GPIO_NUM_2);
    msg.estado_solicitado = (nivel_pin == 1) ? 1 : 0;

    msg.lectura_temperatura = m_temperatura_actual;
    msg.lectura_humedad = m_humedad_actual;
    msg.timestamp_operacion = xTaskGetTickCount() * portTICK_PERIOD_MS;

    esp_err_t err = Node::Espnow::send_report(msg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Reporte enviado a la Central correctamente.");
        m_fallas_seguidas = 0;
        m_estado = FsmState::STATE_IDLE;
    } else {
        ESP_LOGE(TAG, "Fallo al enviar reporte por ESP-NOW (error: %s)", esp_err_to_name(err));
        m_fallas_seguidas++;
        
        if (m_fallas_seguidas >= 3) {
            ESP_LOGW(TAG, "Demasiados fallos de comunicación consecutivos (%lu).", m_fallas_seguidas);
            m_estado = FsmState::STATE_ERROR_RECOVERY;
        } else {
            // Reintentar o volver a IDLE temporalmente
            m_estado = FsmState::STATE_IDLE;
        }
    }
}

void FsmNode::manejarEstadoErrorRecovery() {
    ESP_LOGW(TAG, ">>> FSM: STATE_ERROR_RECOVERY <<<");

    // Intentamos recuperar la comunicación o periféricos
    ESP_LOGI(TAG, "Iniciando protocolo de recuperación. Esperando 5 segundos...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Reiniciar los contadores de falla
    m_fallas_seguidas = 0;
    
    // Intentar volver al ciclo de inicialización completo
    ESP_LOGI(TAG, "Intentando reiniciar FSM...");
    m_estado = FsmState::STATE_INIT;
}
