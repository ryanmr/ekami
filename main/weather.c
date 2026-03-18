#include "weather.h"
#include "config.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdbool.h>
#include <time.h>

static const char *TAG = "weather";

static weather_data_t cached_weather = { .valid = false };

#define MAX_RESPONSE_SIZE 4096
static char response_buf[MAX_RESPONSE_SIZE];
static int response_len;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (response_len + evt->data_len < MAX_RESPONSE_SIZE) {
            memcpy(response_buf + response_len, evt->data, evt->data_len);
            response_len += evt->data_len;
            response_buf[response_len] = '\0';
        }
    }
    return ESP_OK;
}

const char *weather_wind_dir(int deg)
{
    if (deg < 23)  return "N";
    if (deg < 68)  return "NE";
    if (deg < 113) return "E";
    if (deg < 158) return "SE";
    if (deg < 203) return "S";
    if (deg < 248) return "SW";
    if (deg < 293) return "W";
    if (deg < 338) return "NW";
    return "N";
}

const char *weather_aqi_label(int aqi)
{
    switch (aqi) {
    case 1: return "Good";
    case 2: return "Fair";
    case 3: return "Moderate";
    case 4: return "Poor";
    case 5: return "Very Poor";
    default: return "Unknown";
    }
}

weather_data_t weather_fetch(void)
{
    weather_data_t result = { .valid = false };

    char url[320];
    snprintf(url, sizeof(url),
        "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=%s",
        OWM_CITY, OWM_API_KEY, OWM_UNITS);

    ESP_LOGI(TAG, "Fetching weather from OWM for %s", OWM_CITY);

    response_len = 0;
    memset(response_buf, 0, sizeof(response_buf));

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP status: %d, len: %d", status, response_len);

        if (status == 200 && response_len > 0) {
            cJSON *root = cJSON_Parse(response_buf);
            if (root) {
                cJSON *main_obj = cJSON_GetObjectItem(root, "main");
                cJSON *weather_arr = cJSON_GetObjectItem(root, "weather");
                cJSON *wind = cJSON_GetObjectItem(root, "wind");
                cJSON *clouds = cJSON_GetObjectItem(root, "clouds");
                cJSON *sys = cJSON_GetObjectItem(root, "sys");
                cJSON *coord = cJSON_GetObjectItem(root, "coord");

                if (main_obj) {
                    cJSON *t = cJSON_GetObjectItem(main_obj, "temp");
                    cJSON *fl = cJSON_GetObjectItem(main_obj, "feels_like");
                    cJSON *h = cJSON_GetObjectItem(main_obj, "humidity");
                    cJSON *p = cJSON_GetObjectItem(main_obj, "pressure");
                    if (t) result.temp = (float)t->valuedouble;
                    if (fl) result.feels_like = (float)fl->valuedouble;
                    if (h) result.humidity = h->valueint;
                    if (p) result.pressure = p->valueint;
                }

                if (weather_arr && cJSON_GetArraySize(weather_arr) > 0) {
                    cJSON *w0 = cJSON_GetArrayItem(weather_arr, 0);
                    cJSON *desc = cJSON_GetObjectItem(w0, "description");
                    cJSON *ic = cJSON_GetObjectItem(w0, "icon");
                    if (desc && desc->valuestring) {
                        strncpy(result.description, desc->valuestring, sizeof(result.description) - 1);
                        if (result.description[0] >= 'a' && result.description[0] <= 'z')
                            result.description[0] -= 32;
                    }
                    if (ic && ic->valuestring)
                        strncpy(result.icon, ic->valuestring, sizeof(result.icon) - 1);
                }

                if (wind) {
                    cJSON *ws = cJSON_GetObjectItem(wind, "speed");
                    cJSON *wd = cJSON_GetObjectItem(wind, "deg");
                    if (ws) result.wind_speed = (float)ws->valuedouble;
                    if (wd) result.wind_deg = wd->valueint;
                }

                if (clouds) {
                    cJSON *ca = cJSON_GetObjectItem(clouds, "all");
                    if (ca) result.clouds = ca->valueint;
                }

                cJSON *vis = cJSON_GetObjectItem(root, "visibility");
                if (vis) result.visibility = vis->valueint;

                if (sys) {
                    cJSON *sr = cJSON_GetObjectItem(sys, "sunrise");
                    cJSON *ss = cJSON_GetObjectItem(sys, "sunset");
                    if (sr) result.sunrise = (unsigned long)sr->valuedouble;
                    if (ss) result.sunset = (unsigned long)ss->valuedouble;
                }

                if (coord) {
                    cJSON *lat = cJSON_GetObjectItem(coord, "lat");
                    cJSON *lon = cJSON_GetObjectItem(coord, "lon");
                    if (lat) result.lat = (float)lat->valuedouble;
                    if (lon) result.lon = (float)lon->valuedouble;
                }

                result.valid = true;
                ESP_LOGI(TAG, "Weather: %dF (feels %dF), %s, wind %.0f mph %s",
                    (int)result.temp, (int)result.feels_like,
                    result.description, result.wind_speed,
                    weather_wind_dir(result.wind_deg));

                cJSON_Delete(root);
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    if (result.valid) {
        cached_weather = result;
    }

    return result;
}

void weather_fetch_aqi(void)
{
    if (cached_weather.lat == 0.0f) return;

    char url[256];
    snprintf(url, sizeof(url),
        "http://api.openweathermap.org/data/2.5/air_pollution?lat=%.4f&lon=%.4f&appid=%s",
        cached_weather.lat, cached_weather.lon, OWM_API_KEY);

    ESP_LOGI(TAG, "Fetching AQI");

    response_len = 0;
    memset(response_buf, 0, sizeof(response_buf));

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK && esp_http_client_get_status_code(client) == 200 && response_len > 0) {
        cJSON *root = cJSON_Parse(response_buf);
        if (root) {
            cJSON *list = cJSON_GetObjectItem(root, "list");
            if (list && cJSON_GetArraySize(list) > 0) {
                cJSON *item = cJSON_GetArrayItem(list, 0);
                cJSON *main_obj = cJSON_GetObjectItem(item, "main");
                cJSON *comp = cJSON_GetObjectItem(item, "components");
                if (main_obj) {
                    cJSON *a = cJSON_GetObjectItem(main_obj, "aqi");
                    if (a) cached_weather.aqi = a->valueint;
                }
                if (comp) {
                    cJSON *pm = cJSON_GetObjectItem(comp, "pm2_5");
                    if (pm) cached_weather.pm2_5 = (float)pm->valuedouble;
                }
                ESP_LOGI(TAG, "AQI: %d (%s), PM2.5: %.1f",
                    cached_weather.aqi, weather_aqi_label(cached_weather.aqi), cached_weather.pm2_5);
            }
            cJSON_Delete(root);
        }
    }

    esp_http_client_cleanup(client);
}

weather_data_t weather_get_cached(void)
{
    return cached_weather;
}

// NVS cache
void weather_save_cache(void)
{
    nvs_handle_t h;
    if (nvs_open("weather", NVS_READWRITE, &h) != ESP_OK) return;

    nvs_set_i32(h, "temp_x10", (int32_t)(cached_weather.temp * 10));
    nvs_set_i32(h, "feels_x10", (int32_t)(cached_weather.feels_like * 10));
    nvs_set_i32(h, "humidity", cached_weather.humidity);
    nvs_set_str(h, "desc", cached_weather.description);
    nvs_set_str(h, "icon", cached_weather.icon);
    nvs_set_i32(h, "wind_x10", (int32_t)(cached_weather.wind_speed * 10));
    nvs_set_i32(h, "wind_deg", cached_weather.wind_deg);
    nvs_set_i32(h, "pressure", cached_weather.pressure);
    nvs_set_i32(h, "vis", cached_weather.visibility);
    nvs_set_i32(h, "clouds", cached_weather.clouds);
    nvs_set_u32(h, "sunrise", (uint32_t)cached_weather.sunrise);
    nvs_set_u32(h, "sunset", (uint32_t)cached_weather.sunset);
    nvs_set_i32(h, "lat_x1k", (int32_t)(cached_weather.lat * 1000));
    nvs_set_i32(h, "lon_x1k", (int32_t)(cached_weather.lon * 1000));
    nvs_set_i32(h, "aqi", cached_weather.aqi);
    nvs_set_i32(h, "pm25_x10", (int32_t)(cached_weather.pm2_5 * 10));
    nvs_set_u8(h, "valid", cached_weather.valid ? 1 : 0);
    nvs_set_u32(h, "epoch", (uint32_t)time(NULL));

    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Weather cached to NVS");
}

bool weather_load_cache(void)
{
    nvs_handle_t h;
    if (nvs_open("weather", NVS_READONLY, &h) != ESP_OK) return false;

    uint8_t valid = 0;
    nvs_get_u8(h, "valid", &valid);
    if (!valid) { nvs_close(h); return false; }

    int32_t iv;
    uint32_t uv;
    size_t len;

    if (nvs_get_i32(h, "temp_x10", &iv) == ESP_OK) cached_weather.temp = iv / 10.0f;
    if (nvs_get_i32(h, "feels_x10", &iv) == ESP_OK) cached_weather.feels_like = iv / 10.0f;
    if (nvs_get_i32(h, "humidity", &iv) == ESP_OK) cached_weather.humidity = iv;

    len = sizeof(cached_weather.description);
    nvs_get_str(h, "desc", cached_weather.description, &len);
    len = sizeof(cached_weather.icon);
    nvs_get_str(h, "icon", cached_weather.icon, &len);

    if (nvs_get_i32(h, "wind_x10", &iv) == ESP_OK) cached_weather.wind_speed = iv / 10.0f;
    if (nvs_get_i32(h, "wind_deg", &iv) == ESP_OK) cached_weather.wind_deg = iv;
    if (nvs_get_i32(h, "pressure", &iv) == ESP_OK) cached_weather.pressure = iv;
    if (nvs_get_i32(h, "vis", &iv) == ESP_OK) cached_weather.visibility = iv;
    if (nvs_get_i32(h, "clouds", &iv) == ESP_OK) cached_weather.clouds = iv;
    if (nvs_get_u32(h, "sunrise", &uv) == ESP_OK) cached_weather.sunrise = uv;
    if (nvs_get_u32(h, "sunset", &uv) == ESP_OK) cached_weather.sunset = uv;
    if (nvs_get_i32(h, "lat_x1k", &iv) == ESP_OK) cached_weather.lat = iv / 1000.0f;
    if (nvs_get_i32(h, "lon_x1k", &iv) == ESP_OK) cached_weather.lon = iv / 1000.0f;
    if (nvs_get_i32(h, "aqi", &iv) == ESP_OK) cached_weather.aqi = iv;
    if (nvs_get_i32(h, "pm25_x10", &iv) == ESP_OK) cached_weather.pm2_5 = iv / 10.0f;
    cached_weather.valid = true;

    nvs_close(h);
    ESP_LOGI(TAG, "Loaded cached weather: %dF, %s", (int)cached_weather.temp, cached_weather.description);
    return true;
}

bool weather_is_cache_fresh(void)
{
    nvs_handle_t h;
    if (nvs_open("weather", NVS_READONLY, &h) != ESP_OK) return false;

    uint32_t saved_epoch = 0;
    nvs_get_u32(h, "epoch", &saved_epoch);
    nvs_close(h);

    if (saved_epoch == 0) return false;

    time_t now = time(NULL);
    if (now < 1000000) return false;

    uint32_t age = (uint32_t)now - saved_epoch;
    uint32_t max_age = WEATHER_UPDATE_INTERVAL_MS / 1000;
    bool fresh = (age < max_age);
    ESP_LOGI(TAG, "Cache age: %lus, max: %lus -> %s",
        (unsigned long)age, (unsigned long)max_age, fresh ? "fresh" : "stale");
    return fresh;
}
