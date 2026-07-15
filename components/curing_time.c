#include <zephyr/kernel.h>
#include "curing_time.h"
#include "zephyr/device.h"
#include <zephyr/logging/log.h>

#define THRESHOLD_1_TO_5    60

/* Curing time */
static uint16_t program_time = CURING_TIME_DEFAULT_VALUE;
struct k_mutex time_mutex;

uint16_t ct_get_time()
{
    uint16_t time;
    k_mutex_lock(&time_mutex, K_FOREVER);
    time = program_time;
    k_mutex_unlock(&time_mutex);

    return time;
}

void ct_set_time(uint16_t new_time)
{
    if(new_time > CURING_TIME_MAX_VALUE) {
        printk("CT: new_time out of bounds\n");
    } else {
        k_mutex_lock(&time_mutex, K_FOREVER);
        program_time = new_time;
        k_mutex_unlock(&time_mutex);
    }
    
}

void ct_increase_time()
{
    uint8_t value;
    k_mutex_lock(&time_mutex, K_FOREVER);
    if(program_time < THRESHOLD_1_TO_5) {
        value = 1;
    } else {
        value = 5;
    }
    if(program_time + value < CURING_TIME_MAX_VALUE) {
        program_time+=value;
    } else {
        program_time = CURING_TIME_MAX_VALUE;
    }
    k_mutex_unlock(&time_mutex);
}

void ct_decrease_time()
{
    uint8_t value;
    k_mutex_lock(&time_mutex, K_FOREVER);
    if(program_time <= THRESHOLD_1_TO_5) {
        value = 1;
    } else {
        value = 5;
    }
    if(program_time > value) {
        program_time-=value;
    } else {
        program_time = 0;
    }

    k_mutex_unlock(&time_mutex);
}

void ct_mutex_init()
{
    k_mutex_init(&time_mutex);
}