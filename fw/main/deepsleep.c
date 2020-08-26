// Handling of esp32 "deep sleep" modes
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <sys/time.h> // gettimeofday
#include <driver/rtc_io.h> // rtc_gpio_isolate
#include <esp_sleep.h> // esp_deep_sleep_start
#include <esp_wifi.h> // esp_wifi_stop
#include <freertos/FreeRTOS.h> // xTaskCreate
#include <freertos/task.h> // xTaskCreate
#include "deepsleep.h" // deepsleep_init
#include "sdkconfig.h" // CONFIG_MEASURE_INTERVAL

// Time (in us) of last deep sleep enter time
static RTC_DATA_ATTR uint64_t last_deepsleep_time;
// Time (in us) of last wake up time
static uint64_t last_wake_time;
// Is this boot a deepsleep resume?
static int last_wake_from_sleep;

uint64_t
deepsleep_get_wake_time(void)
{
    return last_wake_time;
}

uint64_t
deepsleep_get_sleep_time(void)
{
    return last_deepsleep_time;
}

int
deepsleep_is_wake_from_sleep(void)
{
    return last_wake_from_sleep;
}

// Report time counter in microseconds
static uint64_t
get_usecs(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (uint64_t)now.tv_sec * 1000000 + now.tv_usec;
}

static TaskHandle_t deepsleep_task_id;
static uint64_t force_deepsleep_time;

static void
deepsleep_task(void *pvParameter)
{
    // Wait until ready for deepsleep
    uint32_t sleep_time = CONFIG_MAX_RUN_TIME * 1000 / portTICK_PERIOD_MS;
    for (;;) {
        ulTaskNotifyTake(pdFALSE, sleep_time);
        uint64_t curtime = get_usecs(), fdt = force_deepsleep_time;
        if (curtime >= fdt)
            break;
        sleep_time = (fdt - curtime) / (1000 * portTICK_PERIOD_MS);
    }

    // Enter deepsleep
    esp_wifi_stop();
    esp_deep_sleep_disable_rom_logging();
    esp_sleep_enable_timer_wakeup(CONFIG_MEASURE_INTERVAL * 1000000ULL);
    last_deepsleep_time = get_usecs();
    esp_deep_sleep_start();
}

void
deepsleep_init(void)
{
    last_wake_time = get_usecs();

    int cause = esp_sleep_get_wakeup_cause();
    last_wake_from_sleep = (cause == ESP_SLEEP_WAKEUP_TIMER);

    force_deepsleep_time = last_wake_time + CONFIG_MAX_RUN_TIME * 1000000ULL;
    xTaskCreate(&deepsleep_task, "deepsleep_task", 8192, NULL, 9
                , &deepsleep_task_id);
}

void
deepsleep_note_ota_start(void)
{
    force_deepsleep_time = last_wake_time + CONFIG_MAX_OTA_TIME * 1000000ULL;
}

void
deepsleep_start_sleep(void)
{
    force_deepsleep_time = 0;
    xTaskNotifyGive(deepsleep_task_id);
}

// Permanently enter low power mode (to reduce further battery drain)
void
deepsleep_shutdown(void)
{
    esp_wifi_stop();
    for (int i=0; i<ESP_PD_DOMAIN_MAX; i++)
        esp_sleep_pd_config(i, ESP_PD_OPTION_OFF);
    esp_deep_sleep_start();
}
