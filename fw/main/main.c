// Application start and main loop
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <stdio.h> // snprintf
#include <esp_attr.h> // RTC_DATA_ATTR
#include <esp_log.h> // ESP_LOGI
#include "battery.h" // battery_sense
#include "bme280.h" // bme280_sense
#include "datalog.h" // datalog_init
#include "deepsleep.h" // deepsleep_init
#include "mqtt.h" // mqtt_start
#include "network.h" // network_connect
#include "sdkconfig.h" // CONFIG_UPLOAD_INTERVAL

static const char *TAG = "MQTT_TCP";


/****************************************************************
 * Wake and sleep time reports
 ****************************************************************/

struct appwake_s {
    uint64_t waketime, sleeptime;
};

static int
appwake_format(void *data, char *buf, int size)
{
    struct appwake_s *aw = data;
    const char *latest = "";
    if (aw->waketime == deepsleep_get_wake_time())
        latest = ",\"latest\":1";
    if (!aw->sleeptime)
        return snprintf(buf, size, "\"boot_time\":%llu%s"
                        , aw->waketime, latest);
    return snprintf(buf, size, "\"wake_time\":%llu,\"last_sleep_time\":%llu%s"
                    , aw->waketime, aw->sleeptime, latest);
}

static const struct datalog_type_s appwake_info = {
    .length = sizeof(struct appwake_s),
    .format = appwake_format,
};

static void
waketime_sense(void)
{
    uint64_t waketime = deepsleep_get_wake_time();
    struct appwake_s aw = {
        .waketime = waketime,
        .sleeptime = deepsleep_get_sleep_time()
    };
    datalog_append(&appwake_info, &aw);
}


/****************************************************************
 * Startup
 ****************************************************************/

// Setup debug levels
static void
debug_init(void)
{
    ESP_LOGW(TAG, "[APP] Startup..");
}

static RTC_DATA_ATTR uint64_t next_network_time;

// Main esp32 code start
void
app_main(void)
{
    deepsleep_init();
    debug_init();
    datalog_init();

    waketime_sense();
    battery_sense();
    bme280_sense();
    datalog_finalize();

    // Check if network upload should be attempted
    if (deepsleep_get_wake_time() >= next_network_time) {
        uint64_t upload_interval = CONFIG_UPLOAD_INTERVAL * 1000000;
        next_network_time = deepsleep_get_wake_time() + upload_interval;

        int ret = network_start();
        if (ret)
            goto done;
        mqtt_start();

        network_disconnect();
    }

done:
    deepsleep_start_sleep();
}
