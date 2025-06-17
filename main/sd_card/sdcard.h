#ifndef SDCARD_WOLL
#define SDCARD_WOLL
#include "driver/spi_common.h"
#include <stdint.h>
#include "esp_err.h"

esp_err_t sdcardInit(spi_host_device_t host, int csIo);
void sdcardLogSteps(uint32_t steps);
void sdcardRotateSteps(void);
void sdcardDeleteAllFiles(void);

#endif
