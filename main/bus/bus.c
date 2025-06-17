#include "bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

i2c_master_bus_handle_t busInitI2c(int port, int sdaIo, int sclIo){
    i2c_master_bus_handle_t bus;
    const i2c_master_bus_config_t cfg = {
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port          = port,
        .sda_io_num        = sdaIo,
        .scl_io_num        = sclIo,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&cfg, &bus);
    return bus;
}

void spiInitCs(int csIo){
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << csIo,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(csIo, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void busInitSpi(spi_host_device_t host, int sclkIo, int mosiIo, int misoIo){
    const spi_bus_config_t cfg = {
        .mosi_io_num = mosiIo,
        .miso_io_num = misoIo,
        .sclk_io_num = sclkIo,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(host, &cfg, SPI_DMA_CH_AUTO);
}
