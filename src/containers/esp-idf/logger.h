// Wrapper so that our quick and dirty c logger ext works in esp-idf also
#pragma once

#include "esp_log.h"

#define TAG __FILENAME__
#define LOG_ERROR(fmt, ...)     ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)      ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)      ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)     ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...)     ESP_LOGV(TAG, fmt, ##__VA_ARGS__)