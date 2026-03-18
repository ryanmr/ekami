#include "wifi_manager.h"
#include "config.h"
#include "pcf85063_bsp.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <time.h>

static const char *TAG = "wifi_mgr";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRY          5

static EventGroupHandle_t s_wifi_event_group = NULL;
static int s_retry_num = 0;
static bool s_connected = false;
static bool s_infra_initialized = false;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)", s_retry_num, MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        s_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// One-time infrastructure setup (netif, event loop, event handlers).
// Safe to call multiple times — only runs once.
// NOT thread-safe: must only be called from a single task (app_main or wifi_power_task).
static void wifi_ensure_infra(void)
{
    if (s_infra_initialized) return;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    s_infra_initialized = true;
    ESP_LOGI(TAG, "WiFi infrastructure initialized");
}

bool wifi_manager_init(void)
{
    if (strlen(WIFI_SSID) == 0) {
        ESP_LOGW(TAG, "No WiFi SSID configured, skipping");
        return false;
    }

    wifi_ensure_infra();

    // Create or reset event group
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
    } else {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    }

    s_retry_num = 0;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting...");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
        return true;
    } else {
        ESP_LOGW(TAG, "WiFi connection failed");
        esp_wifi_stop();
        esp_wifi_deinit();
        return false;
    }
}

void wifi_manager_stop(void)
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    s_connected = false;
    ESP_LOGI(TAG, "WiFi stopped");
}

bool wifi_manager_is_connected(void)
{
    return s_connected;
}

void wifi_manager_sync_time(void)
{
    ESP_LOGI(TAG, "Syncing time via SNTP...");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    int retry = 0;
    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < 15) {
        ESP_LOGI(TAG, "Waiting for NTP sync... (%d)", retry);
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry++;
    }

    esp_sntp_stop();

    if (retry >= 15) {
        ESP_LOGW(TAG, "NTP sync timeout");
        return;
    }

    setenv("TZ", TIMEZONE_STR, 1);
    tzset();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "NTP time: %04d-%02d-%02d %02d:%02d:%02d",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    int year_2digit = (timeinfo.tm_year + 1900) % 100;
    PCF85063_SetTime_YMD(year_2digit, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    PCF85063_SetTime_HMS(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    Time_data t = PCF85063_GetTime();
    t.week = timeinfo.tm_wday;
    PCF85063_SetTime(t);

    ESP_LOGI(TAG, "RTC updated from NTP (year=%d, wday=%d)", year_2digit, timeinfo.tm_wday);
}
