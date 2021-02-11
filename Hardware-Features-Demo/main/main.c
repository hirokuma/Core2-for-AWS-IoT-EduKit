#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"

#include "core2forAWS.h"
#include "lvgl/lvgl.h"
#include "ft6336u.h"
#include "bm8563.h"
#include "axp192.h"
#include "cryptoauthlib.h"
#include "i2c_device.h"

static void brightness_slider_event_cb(lv_obj_t * slider, lv_event_t event);
static void strength_slider_event_cb(lv_obj_t * slider, lv_event_t event);

// in disp_spi.c
extern SemaphoreHandle_t xGuiSemaphore;
extern SemaphoreHandle_t spi_mutex;

extern void spi_poll();

void app_main(void)
{
    esp_log_level_set("gpio", ESP_LOG_NONE);
    esp_log_level_set("ILI9341", ESP_LOG_NONE);

    spi_mutex = xSemaphoreCreateMutex();

    Core2ForAWS_Init();
    FT6336U_Init();
    Core2ForAWS_LCD_Init();
    Core2ForAWS_Button_Init();
    BM8563_Init();
    
    rtc_date_t date;
    date.year = 2020;
    date.month = 9;
    date.day = 30;

    date.hour = 13;
    date.minute = 40;
    date.second = 10;    
    BM8563_SetTime(&date);

    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    lv_obj_t * time_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_set_pos(time_label, 10, 5);
    lv_label_set_align(time_label, LV_LABEL_ALIGN_LEFT);

    lv_obj_t * touch_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(touch_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    lv_obj_t * pmu_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(pmu_label, touch_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    lv_obj_t * brightness_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(brightness_label, pmu_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_label_set_text(brightness_label, "Screen brightness");

    lv_obj_t * brightness_slider = lv_slider_create(lv_scr_act(), NULL);
    lv_obj_set_width(brightness_slider, 130);
    lv_obj_align(brightness_slider, brightness_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_set_event_cb(brightness_slider, brightness_slider_event_cb);
    lv_slider_set_value(brightness_slider, 50, LV_ANIM_OFF);
    lv_slider_set_range(brightness_slider, 30, 100);

    lv_obj_t * motor_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(motor_label, brightness_label, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_label_set_text(motor_label, "Motor strength");

    lv_obj_t * strength_slider = lv_slider_create(lv_scr_act(), NULL);
    lv_obj_set_width(strength_slider, 130);
    lv_obj_align(strength_slider, motor_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_event_cb(strength_slider, strength_slider_event_cb);
    lv_slider_set_value(strength_slider, 0, LV_ANIM_OFF);
    lv_slider_set_range(strength_slider, 0, 100);

    xSemaphoreGive(xGuiSemaphore);

    char label_stash[200];
    for (;;) {
        BM8563_GetTime(&date);
        sprintf(label_stash, "Time: %d-%02d-%02d %02d:%02d:%02d\r\n",
                date.year, date.month, date.day, date.hour, date.minute, date.second);
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
        lv_label_set_text(time_label, label_stash);
        xSemaphoreGive(xGuiSemaphore);

        uint16_t x, y;
        bool press;
        FT6336U_GetTouch(&x, &y, &press);
        sprintf(label_stash, "Touch x: %d, y: %d, press: %d\r\n", x, y, press);
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
        lv_label_set_text(touch_label, label_stash);
        xSemaphoreGive(xGuiSemaphore);

        sprintf(label_stash, "Bat %.3f V, %.3f mA\r\n", Core2ForAWS_PMU_GetBatVolt(), Core2ForAWS_PMU_GetBatCurrent());
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
        lv_label_set_text(pmu_label, label_stash);
        xSemaphoreGive(xGuiSemaphore);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void brightness_slider_event_cb(lv_obj_t * slider, lv_event_t event) {
    if(event == LV_EVENT_VALUE_CHANGED) {
        Core2ForAWS_LCD_SetBrightness(lv_slider_get_value(slider));
    }
}

static void strength_slider_event_cb(lv_obj_t * slider, lv_event_t event) {
    if(event == LV_EVENT_VALUE_CHANGED) {
        Core2ForAWS_Motor_SetStrength(lv_slider_get_value(slider));
    }
}
