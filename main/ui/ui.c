#include "ui.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "driver/i2c_master.h"

#define PIN_OLED_RST -1
#define OLED_W 128
#define OLED_H 64

lv_disp_t *initDisplay(i2c_master_bus_handle_t bus, esp_lcd_panel_io_handle_t *oIo, esp_lcd_panel_handle_t *oPanel, uint8_t addr){
    const esp_lcd_panel_io_i2c_config_t ioCfg = {
        .dev_addr            = addr,
        .scl_speed_hz        = 400000,
        .control_phase_bytes = 1,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .dc_bit_offset       = 6,
    };
    esp_lcd_new_panel_io_i2c(bus, &ioCfg, oIo);

    const esp_lcd_panel_ssd1306_config_t ssdCfg = { .height = OLED_H };
    const esp_lcd_panel_dev_config_t pCfg = {
        .bits_per_pixel = 1,
        .reset_gpio_num = PIN_OLED_RST,
        .vendor_config  = &ssdCfg,
    };
    esp_lcd_new_panel_ssd1306(*oIo, &pCfg, oPanel);
    esp_lcd_panel_init(*oPanel);
    esp_lcd_panel_disp_on_off(*oPanel, true);

    lvgl_port_init(&(const lvgl_port_cfg_t)ESP_LVGL_PORT_INIT_CONFIG());
    return lvgl_port_add_disp(&(const lvgl_port_display_cfg_t){
        .io_handle     = *oIo,
        .panel_handle  = *oPanel,
        .buffer_size   = OLED_W * OLED_H,
        .double_buffer = true,
        .hres          = OLED_W,
        .vres          = OLED_H,
        .monochrome    = true,
    });
}

static void cfgLabel(lv_obj_t *lbl, const char *txt){
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_pad_left(lbl, 4, 0);
}

void uiInit(lv_disp_t *disp, ui_t *ui){
    lv_obj_t *scr = lv_disp_get_scr_act(disp);

    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont,
                          LV_FLEX_ALIGN_START, 
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    ui->lblSteps = lv_label_create(cont);
    cfgLabel(ui->lblSteps, "Schritte: 0");

    ui->lblHr = lv_label_create(cont);
    cfgLabel(ui->lblHr, "HF: 0 bpm");

    ui->lblSpo2 = lv_label_create(cont);
    cfgLabel(ui->lblSpo2, "SpO2: 0 %");

    ui->lblStatus = lv_label_create(cont);
    cfgLabel(ui->lblStatus, "SD-Bereit");
}

void uiSetSteps(ui_t *ui, uint32_t steps){
    lv_label_set_text_fmt(ui->lblSteps, "Schritte: %ld", steps);
}

void uiSetHr(ui_t *ui, uint32_t hr){
    lv_label_set_text_fmt(ui->lblHr, "HF: %ld bpm", hr);
}

void uiSetSpo2(ui_t *ui, uint8_t spo2){
    lv_label_set_text_fmt(ui->lblSpo2, "SpO2: %u %%", spo2);
}

void uiSetStatus(ui_t *ui, const char *text){
    lv_label_set_text(ui->lblStatus, text);
}
