#include "spi_sensor.h"
#include "esp_log.h"

static const char* TAG = "SPI_SENSOR";

SpiSensor::SpiSensor(spi_host_device_t host,
                     gpio_num_t mosi_pin,
                     gpio_num_t miso_pin,
                     gpio_num_t sclk_pin,
                     gpio_num_t cs_pin)
    : m_host(host),
      m_mosi_pin(mosi_pin),
      m_miso_pin(miso_pin),
      m_sclk_pin(sclk_pin),
      m_cs_pin(cs_pin),
      m_spi(nullptr) {}

bool SpiSensor::inicializar() {
    if (m_spi != nullptr) {
        return true;
    }

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = m_mosi_pin;
    buscfg.miso_io_num = m_miso_pin;
    buscfg.sclk_io_num = m_sclk_pin;
    buscfg.quadwp_io_num = GPIO_NUM_NC;
    buscfg.quadhd_io_num = GPIO_NUM_NC;
    buscfg.max_transfer_sz = 1;

    esp_err_t err = spi_bus_initialize(m_host, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(err));
        return false;
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 1 * 1000 * 1000; // 1 MHz
    devcfg.mode = 0;
    devcfg.spics_io_num = m_cs_pin;
    devcfg.queue_size = 1;
    devcfg.flags = 0;

    err = spi_bus_add_device(m_host, &devcfg, &m_spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(err));
        spi_bus_free(m_host);
        m_spi = nullptr;
        return false;
    }

    ESP_LOGI(TAG,
             "SPI sensor initialized (host=%d, SCLK=%d, MISO=%d, MOSI=%d, CS=%d)",
             static_cast<int>(m_host),
             static_cast<int>(m_sclk_pin),
             static_cast<int>(m_miso_pin),
             static_cast<int>(m_mosi_pin),
             static_cast<int>(m_cs_pin));
    return true;
}

uint8_t SpiSensor::leerValor() {
    if (m_spi == nullptr) {
        return 0;
    }

    spi_transaction_t trans = {};
    trans.length = 8;
    trans.rxlength = 8;
    trans.flags = SPI_TRANS_USE_RXDATA;

    esp_err_t err = spi_device_transmit(m_spi, &trans);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SPI read failed: %s", esp_err_to_name(err));
        return 0;
    }

    // Convert raw value to 0..100 range. If device returns a larger number, use modulo.
    return static_cast<uint8_t>(trans.rx_data[0] % 101);
}
