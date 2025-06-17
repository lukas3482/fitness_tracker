#include "icm42688.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ICM42688";

// Register
#define REG_DEVICE_CONFIG 0x11   // Soft reset and general config
#define REG_PWR_MGMT0 0x4E       // Enable accelerometer and gyro
#define REG_ACCEL_CONFIG0 0x50   // Accel range
#define REG_ACCEL_DATA_X1 0x1F   // Acceleration data

// Genauigkeit des Beschleunigungssensors
#define ACCEL_SENS_2G 16384.0f

static esp_err_t regWrite(icm42688_t *sensor, uint8_t reg, uint8_t val){
    spi_transaction_t trans = {0};
    trans.flags = SPI_TRANS_USE_TXDATA;
    trans.length = 16;
    trans.tx_data[0] = reg & 0x7F; // MSB=0 → Write  
    trans.tx_data[1] = val;
    return spi_device_polling_transmit(sensor->spi, &trans);
}

static esp_err_t regRead(icm42688_t *sensor, uint8_t reg, uint8_t *buf, size_t len){
    uint8_t tx[1 + len];
    memset(tx, 0, sizeof(tx));
    tx[0] = reg | 0x80;

    spi_transaction_t trans = {0};
    trans.length = 8 * sizeof(tx);
    trans.tx_buffer = tx;
    trans.rx_buffer = tx;

    esp_err_t err = spi_device_polling_transmit(sensor->spi, &trans);
    if (err != ESP_OK)
        return err;

    memcpy(buf, &tx[1], len);
    return ESP_OK;
}

esp_err_t icm42688Init(spi_host_device_t host, int csIoNum, uint32_t clkHz, icm42688_t *outSensor){
    memset(outSensor, 0, sizeof(*outSensor));

    spi_device_interface_config_t devCfg = {
        .clock_speed_hz = (int)clkHz,
        .mode           = 0,
        .spics_io_num   = csIoNum,
        .queue_size     = 1,
    };
    spi_bus_add_device(host, &devCfg, &outSensor->spi);

    regWrite(outSensor, REG_DEVICE_CONFIG, 0x01);
    vTaskDelay(pdMS_TO_TICKS(20));

    regWrite(outSensor, REG_PWR_MGMT0, 0x03);
    vTaskDelay(pdMS_TO_TICKS(10));

    regWrite(outSensor, REG_ACCEL_CONFIG0, (3 << 5) | 0x03);
    vTaskDelay(pdMS_TO_TICKS(10));

    regWrite(outSensor, 0x53, (1 << 3) | (2 << 1));
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "initialised (SPI %u Hz, 8 kHz, ±2 g)", (unsigned)clkHz);
    return ESP_OK;
}

void icm42688ReadAccel(icm42688_t *sensor){
    uint8_t raw[6];
    regRead(sensor, REG_ACCEL_DATA_X1, raw, sizeof(raw));

    sensor->ax = (int16_t)((raw[0] << 8) | raw[1]);
    sensor->ay = (int16_t)((raw[2] << 8) | raw[3]);
    sensor->az = (int16_t)((raw[4] << 8) | raw[5]);
}


bool icm42688StepDetect(icm42688_t *sensor, uint32_t *ioSteps){
    float g = sqrtf((float)sensor->ax * sensor->ax +
                    (float)sensor->ay * sensor->ay +
                    (float)sensor->az * sensor->az) / ACCEL_SENS_2G;

    const float HI = 1.50f;   // Obere-Schwelle
    const float LO = 0.70f;   // Unter-Schwelle

    static bool high = false;

    if (!high && g > HI) {
        high = true;
        (*ioSteps)++;
        return true;
    } else if (high && g < LO) {
        high = false;
    }
    return false;
}
