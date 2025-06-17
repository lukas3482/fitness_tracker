#ifndef BUS_WOLL
#define BUS_WOLL
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

i2c_master_bus_handle_t busInitI2c(int port, int sdaIo, int sclIo);
void spiInitCs(int csIo);
void busInitSpi(spi_host_device_t host, int sclkIo, int mosiIo, int misoIo);

#endif
