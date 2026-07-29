#include <zephyr/kernel.h>
#include "thread_fn.h"

struct k_thread ssd1306_thread;
struct k_thread curing_thread;
struct k_thread time_increase_thread;
struct k_thread time_decrease_thread;
struct k_thread motor_ctrl_thread;

struct k_thread *thread_get_ssd1306()
{
    return &ssd1306_thread;
}

struct k_thread *thread_get_curing()
{
    return &curing_thread;
}

struct k_thread *thread_get_time_increase()
{
    return &time_increase_thread;
}

struct k_thread *thread_get_time_decrease()
{
    return &time_decrease_thread;
}

struct k_thread *thread_get_motor_ctrl()
{
    return &motor_ctrl_thread;
}