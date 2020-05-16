#ifndef DATALOG_H
#define DATALOG_H

struct datalog_type_s {
    int length;
    int (*format)(void *data, char *buf, int size);
};

void datalog_expire(void);
void datalog_finalize(void);
void datalog_append(const struct datalog_type_s *dt, void *data);
int datalog_format(int *ppos, char *buf, int size);
void datalog_init(void);

#endif // battery.h
