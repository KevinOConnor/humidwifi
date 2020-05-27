// Battery voltage sensing
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <stdio.h> // snprintf
#include <driver/adc.h> // adc2_get_raw
#include <driver/gpio.h> // gpio_pullup_en
#include <esp_log.h> // ESP_LOGI
#include "battery.h" // battery_sense
#include "datalog.h" // datalog_append
#include "deepsleep.h" // deepsleep_shutdown
#include "sdkconfig.h" // CONFIG_BATTERY_CHANNEL

static const char *TAG = "BATTERY";

static int
battery_format(void *data, char *buf, int size)
{
    float *fvalue = data;
    return snprintf(buf, size, "\"battery\":%.3f", *fvalue);
}

static const struct datalog_type_s battery_info = {
    .length = sizeof(float),
    .format = battery_format,
};

void
battery_sense(void)
{
    int sense_pin = CONFIG_BATTERY_CHANNEL;
    adc2_config_channel_atten(sense_pin, ADC_ATTEN_DB_6);
    gpio_num_t g;
    adc2_pad_get_io_num(sense_pin, &g);

    gpio_pullup_en(g);
    gpio_pulldown_en(g);

    int value;
    adc2_get_raw(sense_pin, ADC_WIDTH_12Bit, &value);

    gpio_pullup_dis(g);
    gpio_pulldown_dis(g);

    // Calculate a calibrated voltage from adc value
    float scale = atof(CONFIG_BATTERY_SCALE) * (2.2f / 4095.0f);
    float offset = atof(CONFIG_BATTERY_OFFSET);
    float fvalue = value * scale + offset;
    datalog_append(&battery_info, &fvalue);
    ESP_LOGW(TAG, "Got %.3f %d", fvalue, value);

    // Preserve battery if voltage below cutoff
    if (fvalue < atof(CONFIG_BATTERY_CUTOFF))
        deepsleep_shutdown();
}
