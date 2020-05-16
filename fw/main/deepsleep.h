#ifndef DEEPSLEEP_H
#define DEEPSLEEP_H

#include <stdint.h> // uint64_t

uint64_t deepsleep_get_wake_time(void);
uint64_t deepsleep_get_sleep_time(void);
int deepsleep_is_wake_from_sleep(void);
void deepsleep_init(void);
void deepsleep_note_ota_start(void);
void deepsleep_start_sleep(void);
void deepsleep_shutdown(void);

#endif // deepsleep.h
