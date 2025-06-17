#include "max30102.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAX30102";

// Register
#define REG_INTR_ENABLE_1 0x02  // Interrupt enable register 1
#define REG_INTR_ENABLE_2 0x03  // Interrupt enable register 2
#define REG_FIFO_WR_PTR 0x04    // FIFO write pointer
#define REG_OVF_COUNTER 0x05    // Sample overflow counter
#define REG_FIFO_RD_PTR 0x06    // FIFO read pointer
#define REG_FIFO_DATA 0x07      // FIFO data register
#define REG_FIFO_CONFIG 0x08    // FIFO configuration
#define REG_MODE_CONFIG 0x09    // Device mode configuration
#define REG_SPO2_CONFIG 0x0A    // SpO2 measurement settings
#define REG_LED1_PA 0x0C        // LED1 pulse amplitude (RED)
#define REG_LED2_PA 0x0D        // LED2 pulse amplitude (IR)

static esp_err_t regWrite(max30102_t *sensor, uint8_t reg, uint8_t val){
    uint8_t buf[2] = { reg, val };
    return i2c_master_transmit(sensor->dev, buf, sizeof(buf), -1);
}

static esp_err_t regRead(max30102_t *sensor, uint8_t reg, uint8_t *data, size_t len){
    return i2c_master_transmit_receive(sensor->dev, &reg, 1, data, len, -1);
}

esp_err_t max30102Init(i2c_master_bus_handle_t bus, uint8_t addr, max30102_t *outSensor){
    memset(outSensor, 0, sizeof(*outSensor));

    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = 400000,
    };
    i2c_master_bus_add_device(bus, &cfg, &outSensor->dev);

    regWrite(outSensor, REG_MODE_CONFIG, 0x40);
    vTaskDelay(pdMS_TO_TICKS(100));

    regWrite(outSensor, REG_FIFO_WR_PTR, 0x00);
    regWrite(outSensor, REG_OVF_COUNTER, 0x00);
    regWrite(outSensor, REG_FIFO_RD_PTR, 0x00);

    regWrite(outSensor, REG_FIFO_CONFIG, (0b010 << 5) | 0x00);

    regWrite(outSensor, REG_MODE_CONFIG, 0x03);
    regWrite(outSensor, REG_SPO2_CONFIG,
              (0b10 << 5) |   // 100 Hz
              (0b11 << 2) |   // 411 µs / 16-bit 
              0b11);          // Sample Averaging 

    regWrite(outSensor, REG_LED1_PA, 0x3F);
    regWrite(outSensor, REG_LED2_PA, 0x3F);

    regWrite(outSensor, REG_INTR_ENABLE_1, 0x40);
    regWrite(outSensor, REG_INTR_ENABLE_2, 0x00);

    ESP_LOGI(TAG, "MAX30102 initialised");
    return ESP_OK;
}

bool max30102ReadFifo(max30102_t *sensor, uint32_t *red, uint32_t *ir){
    uint8_t raw[6];
    if (regRead(sensor, REG_FIFO_DATA, raw, sizeof(raw)) != ESP_OK) return false;

    *red = ((raw[0] << 16) | (raw[1] << 8) | raw[2]) & 0x03FFFF;
    *ir  = ((raw[3] << 16) | (raw[4] << 8) | raw[5]) & 0x03FFFF;
    return true;
}

static int hillclimbPeakDetection(uint32_t *data, int size, int minDistance, float thrRatio){
    int peaks = 0, lastPeak = -minDistance;
    uint32_t maxVal = 0;
    for (int i = 0; i < size; ++i){
        if (data[i] > maxVal){
            maxVal = data[i];
        } 
    }

    float thr = maxVal * thrRatio;

    for (int i = 1; i < size - 1; ++i) {
        if (data[i] > thr &&
            data[i] > data[i - 1] &&
            data[i] > data[i + 1] &&
            (i - lastPeak) >= minDistance) {
            peaks++;
            lastPeak = i;
        }
    }
    return peaks;
}

void max30102Compute(uint32_t *redBuf, uint32_t *irBuf, int samples, float *oSpo2, int *oBpm){
    /* 1) DC-Anteil */
    double redDc = 0, irDc = 0;
    for (int i = 0; i < samples; ++i) {
        redDc += redBuf[i];
        irDc  += irBuf[i];
    }
    redDc /= samples;
    irDc  /= samples;

    /* 2) AC-Mittelwert */
    double redAc = 0, irAc = 0;
    for (int i = 0; i < samples; ++i) {
        redAc += fabs((double)redBuf[i] - redDc);
        irAc  += fabs((double)irBuf[i]  - irDc);
    }
    redAc /= samples;
    irAc  /= samples;

    // Plausibilitäts-Check
    if (irDc < 10000 || redAc < 20 || irAc < 20) {
        *oSpo2 = 0;
        *oBpm  = 0;
        return;
    }

    // 3) SpO₂-Schätzung nach Verhältnis-Methode
    double R = (redAc / redDc) / (irAc / irDc);
    double spo2 = 110.0 - 25.0 * R;
    if (spo2 < 0)   spo2 = 0;
    if (spo2 > 100) spo2 = 100;
    *oSpo2 = (float)spo2;

    // 4) Herzfrequenz über Peak-Count
    int peaks = hillclimbPeakDetection(irBuf, samples, 15, 0.9f);
    *oBpm = (int)(peaks * 60 / 10);
}
