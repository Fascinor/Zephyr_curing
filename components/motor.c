#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/atomic.h>
#include "thread_fn.h"
#include "motor.h"

#define MOTOR_DELAY_TIME_MS 5000
#define MOTOR_UP_TIME_MS    1000
#define MOTOR_UP_PERIOD_NS  20000

/* Keep a value of 10 * x and < 100 */
static uint8_t motor_pwr_ratio_percent = 80;

/* Motor thread define */
static atomic_t motor_status;
K_THREAD_STACK_DEFINE(motor_ctrl_stack, STACK_SIZE);

static struct k_mutex motor_mutex;

static const struct pwm_dt_spec motor_pwm =
    PWM_DT_SPEC_GET(DT_ALIAS(motor_out));

static void motor_thread_fn(void *arg1, void *arg2, void *arg3)
{
    pwm_set_dt(&motor_pwm, MOTOR_UP_PERIOD_NS, 0);

    while(true)
    {
        if(motor_status)
        {
            pwm_set_dt(&motor_pwm, MOTOR_UP_PERIOD_NS, 0.01 * motor_pwr_ratio_percent * MOTOR_UP_PERIOD_NS);
            k_sleep(K_MSEC(MOTOR_UP_TIME_MS));
            pwm_set_dt(&motor_pwm, MOTOR_UP_PERIOD_NS, 0);
            k_sleep(K_MSEC(MOTOR_DELAY_TIME_MS - MOTOR_UP_TIME_MS));
        } else {
            k_sleep(K_MSEC(100));
        }
    }
}

void motor_init()
{
    k_mutex_init(&motor_mutex);
}

uint8_t motor_get_pwr()
{
    return motor_pwr_ratio_percent;
}

void motor_increase_pwr()
{
    if(motor_pwr_ratio_percent < 100) {
        k_mutex_lock(&motor_mutex, K_FOREVER);
        motor_pwr_ratio_percent += 5;
        k_mutex_unlock(&motor_mutex);
    }
}

void motor_decrease_pwr()
{
    if(motor_pwr_ratio_percent > 0) {
        k_mutex_lock(&motor_mutex, K_FOREVER);
        motor_pwr_ratio_percent -= 5;
        k_mutex_unlock(&motor_mutex);
    }
}

void motor_thread_create()
{
    motor_deactivate();
    k_thread_create(thread_get_motor_ctrl(), motor_ctrl_stack, STACK_SIZE, motor_thread_fn, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
}

void motor_deactivate()
{
    atomic_set(&motor_status, 0);
}

void motor_activate()
{
    atomic_set(&motor_status, 1);
}