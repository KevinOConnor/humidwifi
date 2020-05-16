// Store a log of sensor data in esp32 "rtc memory"
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <stdint.h> // uint8_t
#include <string.h> // memcpy
#include <esp_attr.h> // RTC_DATA_ATTR
#include <esp_log.h> // ESP_LOGI
#include "datalog.h" // datalog_init

static RTC_DATA_ATTR uint8_t log_storage[3600];
static RTC_DATA_ATTR uint16_t log_first, log_end;

struct log_header_s {
    uint8_t length;
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static inline int pos_wrap(int pos) {
    return pos < ARRAY_SIZE(log_storage) ? pos : pos - ARRAY_SIZE(log_storage);
}
static inline int log_avail(void) {
    int used = log_end - log_first;
    return used >= 0 ? ARRAY_SIZE(log_storage) - used : -(used + 1);
}

void
datalog_expire(void)
{
    if (log_first != log_end)
        log_first = pos_wrap(log_first + log_storage[log_first]);
}

void
datalog_finalize(void)
{
    log_end = pos_wrap(log_end + log_storage[log_end]);
}

static void
raw_append(int log_pending, void *data, int len)
{
    int new_len = log_pending + len;
    if (new_len > 255)
        return;
    while (log_avail() < new_len)
        datalog_expire();
    int dest_pos = pos_wrap(log_end + log_pending);
    int seq_space = ARRAY_SIZE(log_storage) - dest_pos;
    if (len > seq_space) {
        memcpy(&log_storage[dest_pos], data, seq_space);
        data += seq_space;
        len -= seq_space;
        dest_pos = 0;
    }
    memcpy(&log_storage[dest_pos], data, len);
    log_storage[log_end] = new_len;
}

static void
raw_pull(void *dest, int src_pos, int len)
{
    int seq_space = ARRAY_SIZE(log_storage) - src_pos;
    if (len > seq_space) {
        memcpy(dest, &log_storage[src_pos], seq_space);
        dest += seq_space;
        len -= seq_space;
        src_pos = 0;
    }
    memcpy(dest, &log_storage[src_pos], len);
}

void
datalog_append(const struct datalog_type_s *dt, void *data)
{
    int dt_len = dt->length, log_pending = log_storage[log_end];
    raw_append(log_pending, &dt, sizeof(dt));
    raw_append(log_pending + sizeof(dt), data, dt_len);
}

int
datalog_format(int *ppos, char *buf, int size)
{
    int pos = *ppos;
    if (pos < 0)
        pos = log_first;
    if (pos == log_end)
        return -1;
    int len = log_storage[pos];
    *ppos = pos_wrap(pos + len);
    if (size < 2)
        return 0;
    pos++;
    len--;
    char *orig_buf = buf;
    *buf++ = '{';
    size--;
    while (len > 0) {
        const struct datalog_type_s *dt;
        raw_pull(&dt, pos_wrap(pos), sizeof(dt));
        int dt_len = dt->length;
        uint8_t __aligned(sizeof(void*)) data[dt_len];
        raw_pull(data, pos_wrap(pos + sizeof(dt)), dt_len);
        int ret = dt->format(data, buf, size);
        if (ret >= size)
            return 0;
        buf += ret;
        *buf++ = ',';
        size -= ret + 1;
        pos += sizeof(dt) + dt_len;
        len -= sizeof(dt) + dt_len;
    }
    buf[-1] = '}';
    return buf - orig_buf;
}

void
datalog_init(void)
{
    struct log_header_s hdr = { .length = 1 };
    raw_append(0, &hdr, sizeof(hdr));
}
