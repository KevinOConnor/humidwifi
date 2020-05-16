// Flash over-the-air code updates
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <string.h> // memcpy
#include <esp_http_client.h> // esp_http_client_config_t
#include <esp_https_ota.h> // esp_https_ota
#include <esp_log.h> // ESP_LOGD
#include <freertos/task.h> // xTaskCreate
#include "ota.h" // ota_start

static const char *TAG = "OTA";

static void
simple_ota_example_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA updat");

    esp_http_client_config_t config = {
        .url = pvParameter,
    };
    esp_err_t ret = esp_https_ota(&config);
    if (ret)
        ESP_LOGE(TAG, "Firmware upgrade failed");

    esp_restart();
}

void
ota_start(char *url, int url_len)
{
    char *new_url = malloc(url_len+1);
    memcpy(new_url, url, url_len);
    new_url[url_len] = 0;

    xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192
                , new_url, 5, NULL);
}
