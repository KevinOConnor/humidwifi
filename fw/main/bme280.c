// BME280 humidity sensor support
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <driver/i2c.h> // i2c_param_config
#include <esp_attr.h> // RTC_DATA_ATTR
#include <esp_log.h> // ESP_LOGW
#include "bme280.h" // bme280_sense
#include "datalog.h" // datalog_append
#include "deepsleep.h" // deepsleep_is_wake_from_sleep
#include "sdkconfig.h" // CONFIG_BME280_SDA_GPIO

#define I2C_FREQUENCY 100000

static const char *TAG = "BME280";


/****************************************************************
 * I2C helpers
 ****************************************************************/

static int
i2c_init(void)
{
    int ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret)
        return ret;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_BME280_SDA_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = CONFIG_BME280_SCL_GPIO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQUENCY,
    };
    return i2c_param_config(I2C_NUM_0, &conf);
}

static int
i2c_write(uint8_t reg, uint8_t data)
{
    int i2c_addr = CONFIG_BME280_I2C_ADDR;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_addr << 1 | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_write_byte(cmd, data, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static int
i2c_read(uint8_t reg, uint8_t *data, int len)
{
    int i2c_addr = CONFIG_BME280_I2C_ADDR;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_addr << 1 | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_addr << 1 | I2C_MASTER_READ, 1);
    if (len > 1)
        i2c_master_read(cmd, data, len - 1, 0);
    i2c_master_read_byte(cmd, &data[len - 1], 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


/****************************************************************
 * BME280 calibration and config
 ****************************************************************/

struct bme280_calibration_s {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    uint8_t dig_H3;
    int16_t dig_H2;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
};
static RTC_DATA_ATTR struct bme280_calibration_s calib;

static inline uint16_t load_short(uint8_t *p) {
    return (p[1] << 8) | p[0];
}

static int
bme280_configure(void)
{
    // Read calibration data
    uint8_t data[0x9f - 0x88 + 1];
    int ret = i2c_read(0x88, data, sizeof(data));
    if (ret)
        goto fail;
    calib.dig_T1 = load_short(&data[0]);
    calib.dig_T2 = load_short(&data[2]);
    calib.dig_T3 = load_short(&data[4]);
    calib.dig_P1 = load_short(&data[6]);
    calib.dig_P2 = load_short(&data[8]);
    calib.dig_P3 = load_short(&data[10]);
    calib.dig_P4 = load_short(&data[12]);
    calib.dig_P5 = load_short(&data[14]);
    calib.dig_P6 = load_short(&data[16]);
    calib.dig_P7 = load_short(&data[18]);
    calib.dig_P8 = load_short(&data[20]);
    calib.dig_P9 = load_short(&data[22]);
    ret = i2c_read(0xA1, &calib.dig_H1, 1);
    if (ret)
        goto fail;
    ret = i2c_read(0xE1, data, 0xe7 - 0xe1 + 1);
    if (ret)
        goto fail;
    calib.dig_H2 = load_short(&data[0]);
    calib.dig_H3 = data[2];
    calib.dig_H4 = (data[3] << 4) | (data[4] & 0x0f);
    calib.dig_H5 = (data[4] >> 4) | (data[5] << 4);
    calib.dig_H6 = data[6];

    // Setup humidity control
    uint8_t ctrl_hum = 0x1;
    ret = i2c_write(0xf2, ctrl_hum);
    if (ret)
        goto fail;

    return 0;

fail:
    ESP_LOGW(TAG, "bme280_configure error %d", ret);
    return ret;
}

// Calculate temperature (formula from bme280 spec)
static int32_t
bme280_calc_t_fine(uint8_t *data)
{
    int32_t adc_T = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int32_t dig_T1 = calib.dig_T1, dig_T2 = calib.dig_T2, dig_T3 = calib.dig_T3;

    int32_t var1 = (((adc_T >> 3) - (dig_T1 << 1)) * dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12)
                    * dig_T3) >> 14;
    return var1 + var2;
}

// Convert temperature to float
static float
bme280_calc_temp(int32_t t_fine)
{
    return t_fine / 5120.0f;
}

// Calculate pressure (formula from bme280 spec)
static float
bme280_calc_pressure(int32_t t_fine, uint8_t *data)
{
    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int64_t dig_P1 = calib.dig_P1, dig_P2 = calib.dig_P2, dig_P3 = calib.dig_P3;
    int64_t dig_P4 = calib.dig_P4, dig_P5 = calib.dig_P5, dig_P6 = calib.dig_P6;
    int64_t dig_P7 = calib.dig_P7, dig_P8 = calib.dig_P8, dig_P9 = calib.dig_P9;

    int64_t var1 = (int64_t)t_fine - 128000;
    int64_t var2 = var1 * var1 * dig_P6;
    var2 = var2 + ((var1 * dig_P5) << 17);
    var2 = var2 + (dig_P4 << 35);
    var1 = ((var1 * var1 * dig_P3) >> 8) + ((var1 * dig_P2) << 12);
    var1 = (((1LL << 47) + var1) * dig_P1) >> 33;
    if (!var1)
        return 0.;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = (dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (dig_P7 << 4);

    return (uint32_t)p / 25600.0f;
}

// Calculate humidity (formula from bme280 spec)
static float
bme280_calc_humidity(int32_t t_fine, uint8_t *data)
{
    int32_t adc_H = (data[0] << 8) | data[1];
    int32_t dig_H1 = calib.dig_H1, dig_H2 = calib.dig_H2, dig_H3 = calib.dig_H3;
    int32_t dig_H4 = calib.dig_H4, dig_H5 = calib.dig_H5, dig_H6 = calib.dig_H6;

    int32_t v_x1_u32r = t_fine - 76800;
    v_x1_u32r =
        ((((adc_H << 14) - (dig_H4 << 20) - (dig_H5 * v_x1_u32r)) + 16384) >> 15)
        * (((((((v_x1_u32r * dig_H6) >> 10)
               * (((v_x1_u32r * dig_H3) >> 11) + 32768)) >> 10) + 2097152)
            * dig_H2 + 8192) >> 14);
    v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15)
                                * (v_x1_u32r >> 15)) >> 7) * dig_H1) >> 4);
    v_x1_u32r = v_x1_u32r < 0 ? 0 : v_x1_u32r;
    v_x1_u32r = v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r;

    return (v_x1_u32r >> 12) / 1024.0f;
}


/****************************************************************
 * BME280 datalog entry
 ****************************************************************/

struct bme280_s {
    float temperature, pressure, humidity;
};

static int
bme280_format(void *data, char *buf, int size)
{
    struct bme280_s *b = data;
    return snprintf(buf, size
                    , "\"temperature\":%.2f,\"pressure\":%.1f,\"humidity\":%.1f"
                    , b->temperature, b->pressure, b->humidity);
}

const struct datalog_type_s bme280_info = {
    .length = sizeof(struct bme280_s),
    .format = bme280_format,
};


/****************************************************************
 * Sensing
 ****************************************************************/

static RTC_DATA_ATTR uint8_t did_init;

void
bme280_sense(void)
{
    int ret = i2c_init();
    if (ret)
        goto fail;

    if (!did_init) {
        ret = bme280_configure();
        if (ret)
            goto fail;
        did_init = 1;
    }

    // XXX - configure oversampling?

    // Request measurement
    uint8_t cmd = (0x1 << 2) | (0x1 << 5) | 0x1;
    ret = i2c_write(0xf4, cmd);
    if (ret)
        goto fail;
    vTaskDelay(18 / portTICK_PERIOD_MS); // XXX

    // Read data from sensor
    uint8_t data[8];
    ret = i2c_read(0xf7, data, sizeof(data));
    if (ret)
        goto fail;

    // Calculate calibrated data and add to datalog
    struct bme280_s b;
    int32_t t_fine = bme280_calc_t_fine(&data[3]);
    b.temperature = bme280_calc_temp(t_fine);
    b.pressure = bme280_calc_pressure(t_fine, &data[0]);
    b.humidity = bme280_calc_humidity(t_fine, &data[6]);
    ESP_LOGW(TAG, "append %.2f %.1f %.1f"
             , b.temperature, b.pressure, b.humidity);
    datalog_append(&bme280_info, &b);
    return;

fail:
    ESP_LOGW(TAG, "bme280_sense error %d", ret);
}
