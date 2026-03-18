#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "config.h"
#include "badge_mode.h"
#include "clock_mode.h"
#include "weather_detail_mode.h"
#include "sun_arc_mode.h"
#include "moon_mode.h"
#include "debug_mode.h"
#include "weather.h"
#include "wifi_manager.h"

// Hardware BSP
#include "epaper_port.h"
#include "GUI_Paint.h"
#include "i2c_bsp.h"
#include "pcf85063_bsp.h"
#include "shtc3_bsp.h"
#include "button_bsp.h"
#include "axp_prot.h"

static const char *TAG = "ekami";

typedef enum {
    MODE_BADGE = 0,
    MODE_CLOCK,
    MODE_WEATHER_DETAIL,
    MODE_SUN_ARC,
    MODE_MOON,
    MODE_DEBUG,
    MODE_COUNT,
} ekami_mode_t;

static uint8_t *image_buf = NULL;
static ekami_mode_t current_mode = MODE_BADGE;

static void render_current_mode(void)
{
    EPD_Init();
    switch (current_mode) {
    case MODE_BADGE:
        badge_mode_render(image_buf, current_mode, MODE_COUNT);
        break;
    case MODE_CLOCK:
        clock_mode_render(image_buf, current_mode, MODE_COUNT);
        break;
    case MODE_WEATHER_DETAIL:
        weather_detail_mode_render(image_buf, current_mode, MODE_COUNT);
        break;
    case MODE_SUN_ARC:
        sun_arc_mode_render(image_buf, current_mode, MODE_COUNT);
        break;
    case MODE_MOON:
        moon_mode_render(image_buf, current_mode, MODE_COUNT);
        break;
    case MODE_DEBUG:
        debug_mode_render(image_buf, current_mode, MODE_COUNT);
        break;
    default:
        break;
    }
}

// Smart WiFi power task — on battery: 2-min window every 30min
static void wifi_power_task(void *arg)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(WEATHER_UPDATE_INTERVAL_MS));

        if (get_usb_connected()) {
            // On USB: WiFi stays on, just fetch
            if (wifi_manager_is_connected()) {
                weather_fetch();
                weather_fetch_aqi();
                weather_save_cache();
            }
        } else {
            // On battery: wake WiFi, fetch, then sleep WiFi
            ESP_LOGI(TAG, "Battery mode: WiFi wake window");
            bool ok = wifi_manager_init();
            if (ok) {
                wifi_manager_sync_time();
                weather_fetch();
                weather_fetch_aqi();
                weather_save_cache();
                vTaskDelay(pdMS_TO_TICKS(2000));
                wifi_manager_stop();
            } else {
                ESP_LOGW(TAG, "WiFi connect failed, skipping this window");
            }
        }
    }
}

static void ui_task(void *arg)
{
    render_current_mode();

    int full_refresh_count = 0;
    int ticks_per_update = CLOCK_UPDATE_INTERVAL_MS / 1000;
    int tick_count = 0;

    while (1) {
        int button = wait_key_event_and_return_code(pdMS_TO_TICKS(1000));

        // Rotary CW (next)
        if (button == 14 || button == 7) {
            current_mode = (current_mode + 1) % MODE_COUNT;
            ESP_LOGI(TAG, "Mode -> %d", current_mode);
            render_current_mode();
            tick_count = 0;
            full_refresh_count = 0;
            continue;
        }

        // Rotary CCW (previous)
        if (button == 0) {
            current_mode = (current_mode - 1 + MODE_COUNT) % MODE_COUNT;
            ESP_LOGI(TAG, "Mode -> %d", current_mode);
            render_current_mode();
            tick_count = 0;
            full_refresh_count = 0;
            continue;
        }

        // Power off
        if (button == 22) {
            ESP_LOGI(TAG, "Power off");
            EPD_Sleep();
            axp_pwr_off();
            continue;
        }

        // Force full refresh
        if (button == 12) {
            ESP_LOGI(TAG, "Manual full refresh");
            render_current_mode();
            tick_count = 0;
            full_refresh_count = 0;
            continue;
        }

        // Clock mode periodic updates
        if (current_mode == MODE_CLOCK) {
            tick_count++;
            if (tick_count >= ticks_per_update) {
                tick_count = 0;
                full_refresh_count++;

                int partials_before_full = FULL_REFRESH_INTERVAL_MS / CLOCK_UPDATE_INTERVAL_MS;
                if (full_refresh_count >= partials_before_full) {
                    EPD_Init();
                    clock_mode_render(image_buf, current_mode, MODE_COUNT);
                    full_refresh_count = 0;
                } else {
                    EPD_Init();
                    clock_mode_update_time(image_buf, current_mode, MODE_COUNT);
                }
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ekami digital badge starting");

    ESP_ERROR_CHECK(nvs_flash_init());

    i2c_master_init();
    vTaskDelay(pdMS_TO_TICKS(50));
    i2c_devices_init();
    vTaskDelay(pdMS_TO_TICKS(50));

    axp_init();
    PCF85063_init();
    i2c_shtc3_init();
    button_Init();

    epaper_port_init();
    EPD_Init();
    EPD_Clear();
    vTaskDelay(pdMS_TO_TICKS(500));

    image_buf = (uint8_t *)heap_caps_malloc(EPD_SIZE_MONO, MALLOC_CAP_SPIRAM);
    if (image_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate image buffer!");
        return;
    }

    // Initialize weather subsystem and load cache (before WiFi)
    weather_init();
    weather_load_cache();

    // Initial WiFi connect + weather fetch
    bool wifi_ok = wifi_manager_init();
    if (wifi_ok) {
        wifi_manager_sync_time();
        weather_fetch();
        weather_fetch_aqi();
        weather_save_cache();

        // If on battery, disconnect WiFi after initial fetch
        if (!get_usb_connected()) {
            ESP_LOGI(TAG, "Battery mode: disconnecting WiFi after init");
            wifi_manager_stop();
        }
    }

    // Start background tasks
    xTaskCreate(wifi_power_task, "wifi_pwr", 8 * 1024, NULL, 2, NULL);
    xTaskCreate(ui_task, "ui", 32 * 1024, NULL, 3, NULL);

    ESP_LOGI(TAG, "ekami initialized (USB=%s)", get_usb_connected() ? "yes" : "no");

    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}
