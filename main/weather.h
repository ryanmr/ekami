#ifndef EKAMI_WEATHER_H
#define EKAMI_WEATHER_H

#include <stdbool.h>

typedef struct {
    float temp;
    float feels_like;
    int humidity;
    char description[32];
    char icon[8];
    float wind_speed;
    int wind_deg;
    int pressure;
    int visibility;
    int clouds;
    unsigned long sunrise;
    unsigned long sunset;
    float lat, lon;
    int aqi;
    float pm2_5;
    bool valid;
} weather_data_t;

// Initialize weather subsystem (must be called once from app_main)
void weather_init(void);

// Fetch current weather from OpenWeatherMap
weather_data_t weather_fetch(void);

// Fetch AQI data (requires valid lat/lon from prior weather fetch)
void weather_fetch_aqi(void);

// Get cached weather data (no HTTP request)
weather_data_t weather_get_cached(void);

// NVS persistence
void weather_save_cache(void);
bool weather_load_cache(void);
bool weather_is_cache_fresh(void);

// Wind direction string from degrees
const char *weather_wind_dir(int deg);

// AQI label from 1-5 scale
const char *weather_aqi_label(int aqi);

#endif // EKAMI_WEATHER_H
