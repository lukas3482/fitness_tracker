#include "sdcard.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

static const char *TAG = "sdcard";
#define MOUNT_POINT "/sdcard"
static sdmmc_card_t *card;

esp_err_t sdcardInit(spi_host_device_t host, int csIo){
    sdmmc_host_t spiHost = SDSPI_HOST_DEFAULT();
    spiHost.slot = host;

    sdspi_device_config_t slotCfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slotCfg.gpio_cs = csIo;
    slotCfg.host_id = host;

    esp_vfs_fat_mount_config_t mountCfg = {
        .format_if_mount_failed = false,
        .max_files = 2
    };

    esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &spiHost, &slotCfg, &mountCfg, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

static uint32_t rotateIdx = 1;

void sdcardRotateSteps(void){
    char oldPath[32];
    char newPath[48];
    snprintf(oldPath, sizeof(oldPath), "%s/steps.csv", MOUNT_POINT);
    while (1) {
        snprintf(newPath, sizeof(newPath), "%s/steps_%lu.csv", MOUNT_POINT, (unsigned long)rotateIdx);
        if (access(newPath, F_OK) != 0) {
            break;
        }
        rotateIdx++;
    }
    if (rename(oldPath, newPath) != 0) {
        ESP_LOGE(TAG, "rename failed");
    } else {
        rotateIdx++;
    }
}

void sdcardDeleteAllFiles(void){
    DIR *dir = opendir(MOUNT_POINT);
    if (!dir) {
        ESP_LOGE(TAG, "opendir failed");
        return;
    }

    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (strncmp(e->d_name, "STEPS", 5) == 0) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", MOUNT_POINT, e->d_name);
            unlink(path);
        }
    }
    closedir(dir);
}

void sdcardLogSteps(uint32_t steps){
    char path[32];
    snprintf(path, sizeof(path), "%s/steps.csv", MOUNT_POINT);
    FILE *f = fopen(path, "a");
    if (!f) {
        ESP_LOGE(TAG, "open failed");
        return;
    }

    fprintf(f, "%lu\n", steps);
    fclose(f);
}
