#ifndef UI_WOLL
#define UI_WOLL
#include "lvgl.h"
#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

typedef struct {
    lv_obj_t *lblSteps;
    lv_obj_t *lblHr;
    lv_obj_t *lblSpo2;
    lv_obj_t *lblStatus;
} ui_t;

void uiInit(lv_disp_t *disp, ui_t *ui);

lv_disp_t *initDisplay(i2c_master_bus_handle_t bus, esp_lcd_panel_io_handle_t *io, esp_lcd_panel_handle_t *panel, uint8_t addr);

void uiSetSteps(ui_t *ui, uint32_t steps);
void uiSetHr   (ui_t *ui, uint32_t hr);
void uiSetSpo2 (ui_t *ui, uint8_t  spo2);
void uiSetStatus(ui_t *ui, const char *text);

#endif
