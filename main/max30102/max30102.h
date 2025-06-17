#ifndef MAX30102_WOLL
#define MAX30102_WOLL

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

typedef struct {
    i2c_master_dev_handle_t dev;
} max30102_t;

esp_err_t max30102Init(i2c_master_bus_handle_t bus, uint8_t addr, max30102_t *outSensor);
bool max30102ReadFifo(max30102_t *sensor, uint32_t *red, uint32_t *ir);
void max30102Compute(uint32_t *redBuf, uint32_t *irBuf, int samples, float *oSpo2, int *oBpm);

#endif
