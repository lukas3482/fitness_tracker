#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ui/ui.h"
#include "max30102.h"
#include "icm42688.h"
#include "bus.h"
#include "sdcard.h"

static const char *TAG = "app";
#define DEBUG false

// Pin Definitionen
#define I2C_PORT 0
#define PIN_SDA 5
#define PIN_SCL 6

#define SPI_HOST SPI2_HOST
#define PIN_IMU_SCLK 10
#define PIN_IMU_MOSI 4
#define PIN_IMU_MISO 7
#define PIN_IMU_CS 1
#define PIN_SD_CS 0
#define PIN_BTN_RST 9
#define PIN_BTN_DEL 2

#define MAX30102_ADDR 0x57
#define OLED_ADDR 0x3C


uint32_t steps = 0;
uint32_t stepsLastLog = 0;
TickType_t tLastLog = 0;
int bpm = 0;
float spo2 = 0.0f;

static volatile bool btnRstFlag = false;
static volatile bool btnDelFlag = false;

static void btnReset(){
    btnRstFlag = true;
}

static void btnDelete(){
    btnDelFlag = true;
}

static void initSensors(i2c_master_bus_handle_t bus, max30102_t *max, icm42688_t *icm){
    max30102Init(bus, MAX30102_ADDR, max);
    icm42688Init(SPI_HOST, PIN_IMU_CS, 4 * 1000 * 1000, icm);
}

static void initGPIOs(){
    gpio_config_t btnCfg = {
        .pin_bit_mask = (1ULL << PIN_BTN_RST) | (1ULL << PIN_BTN_DEL),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btnCfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BTN_RST, btnReset, NULL);
    gpio_isr_handler_add(PIN_BTN_DEL, btnDelete, NULL);
}

uint32_t redBuf [400];
uint32_t irBuf [400];

void addtoBuffer(uint32_t redValue, uint32_t irValue){
    for (int i = 400 - 1; i > 0; i--) {
        redBuf[i] = redBuf[i - 1];
        irBuf[i] = irBuf[i - 1];
    }
    redBuf[0] = redValue;
    irBuf[0] = irValue;
}


void app_main(void){
    // Start Init
    spiInitCs(PIN_IMU_CS);
    spiInitCs(PIN_SD_CS);
    i2c_master_bus_handle_t i2cBus = busInitI2c(I2C_PORT, PIN_SDA, PIN_SCL);
    busInitSpi(SPI_HOST, PIN_IMU_SCLK, PIN_IMU_MOSI, PIN_IMU_MISO);

    bool sd_ok = sdcardInit(SPI_HOST, PIN_SD_CS) == ESP_OK;

    esp_lcd_panel_io_handle_t ioHandle;
    esp_lcd_panel_handle_t panelHandle;
    lv_disp_t *disp = initDisplay(i2cBus, &ioHandle, &panelHandle, OLED_ADDR);

    static ui_t ui;
    if (lvgl_port_lock(portMAX_DELAY)) {
        uiInit(disp, &ui);
        lvgl_port_unlock();
    }

    initGPIOs();

    static max30102_t max;
    static icm42688_t icm;
    initSensors(i2cBus, &max, &icm);

    
    tLastLog = xTaskGetTickCount();

    while (true) {
        // Puls und SpO2 Messung
        uint32_t redValue, irValue;
        max30102ReadFifo(&max, &redValue, &irValue);
        addtoBuffer(redValue, irValue);
        max30102Compute(redBuf, irBuf, 400, &spo2, &bpm);

        // Schrittzähler
        icm42688ReadAccel(&icm);
        if(DEBUG){
            float g = sqrtf((float)icm.ax * icm.ax +
                            (float)icm.ay * icm.ay +
                            (float)icm.az * icm.az) / 16384.0f;
            ESP_LOGI(TAG, "g = %.2f", g);
        }
        icm42688StepDetect(&icm, &steps);
    
        // Buttons
        if (btnRstFlag) {
            btnRstFlag = false;
            sdcardRotateSteps();
            steps = 0;
            stepsLastLog = 0;
            ESP_LOGI(TAG, "Schritte zurueckgesetzt.");
        }

        if (btnDelFlag) {
            btnDelFlag = false;
            sdcardDeleteAllFiles();
            ESP_LOGI(TAG, "Alle Schritt Daten von SD-Karte geloescht.");
        }

        // UI
        if (lvgl_port_lock(portMAX_DELAY)) {
            uiSetSteps(&ui, steps);
            uiSetHr(&ui, bpm);
            uiSetSpo2(&ui, spo2);
            if (sd_ok) {
                uiSetStatus(&ui, "SD-Bereit");
            }else{
                uiSetStatus(&ui, "SD-Fehler");
            }
            lvgl_port_unlock();
        }

        // Schreibe die gezählten Schritte der letzten Minute auf die SD-Karte
        if (sd_ok && xTaskGetTickCount() - tLastLog >= pdMS_TO_TICKS(60000)) {
            sdcardLogSteps(steps - stepsLastLog);
            stepsLastLog = steps;
            tLastLog = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
