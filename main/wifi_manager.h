#ifndef EKAMI_WIFI_MANAGER_H
#define EKAMI_WIFI_MANAGER_H

#include <stdbool.h>

// Initialize WiFi in station mode and connect using credentials from config.h
// Returns true if connected successfully.
bool wifi_manager_init(void);

// Disconnect WiFi and free resources
void wifi_manager_stop(void);

// Check if WiFi is currently connected
bool wifi_manager_is_connected(void);

// Sync system time via SNTP, then set the RTC
void wifi_manager_sync_time(void);

#endif // EKAMI_WIFI_MANAGER_H
