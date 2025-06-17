#ifndef ICM_42688_WOLL
#define ICM_42688_WOLL
#include "driver/spi_master.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    spi_device_handle_t spi; 
    int16_t ax, ay, az;        
} icm42688_t;


esp_err_t icm42688Init(spi_host_device_t host, int csIoNum, uint32_t clkHz, icm42688_t *outSensor);

void icm42688ReadAccel(icm42688_t *sensor);

bool icm42688StepDetect(icm42688_t *sensor, uint32_t *ioStepCnt);

#endif
