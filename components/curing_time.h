#ifndef CURING_TIME_H
#define CURING_TIME_H

#define CURING_TIME_MAX_VALUE			3000
#define CURING_TIME_DEFAULT_VALUE		60

uint16_t ct_get_time();

void ct_set_time(uint16_t new_time);

void ct_increase_time();

void ct_decrease_time();

void ct_mutex_init();

#endif //   CURING_TIME_H