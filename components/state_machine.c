#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include "state_machine.h"
#include <zephyr/sys/printk.h>
#include <zephyr/sys/atomic.h>
#include "thread_fn.h"

#define MOTOR_DELAY_TIME_MS 5000
#define MOTOR_UP_TIME_MS    300
#define MOTOR_UP_PERIOD_NS  20000
#define MOTOR_UP_RATIO      MOTOR_UP_PERIOD_NS * 0.8

/* Motor thread define */
static atomic_t motor_thread_status;
K_THREAD_STACK_DEFINE(motor_ctrl_stack, STACK_SIZE);

/* Outputs */
static const struct gpio_dt_spec spot_out =
    GPIO_DT_SPEC_GET(DT_ALIAS(spot_out), gpios);

static const struct gpio_dt_spec strap_out =
    GPIO_DT_SPEC_GET(DT_ALIAS(strap_out), gpios);

static const struct pwm_dt_spec motor_pwm =
    PWM_DT_SPEC_GET(DT_ALIAS(motor_out));

static curing_state_t current_state = time_selection;
static struct k_mutex state_mutex;

static void sm_motor_control(void *arg1, void *arg2, void *arg3)
{
    pwm_set_dt(&motor_pwm, MOTOR_UP_PERIOD_NS, 0);

    while(true)
    {
        if(motor_thread_status)
        {
            pwm_set_dt(&motor_pwm, MOTOR_UP_PERIOD_NS, MOTOR_UP_RATIO);
            k_sleep(K_MSEC(MOTOR_UP_TIME_MS));
            pwm_set_dt(&motor_pwm, MOTOR_UP_PERIOD_NS, 0);
            k_sleep(K_MSEC(MOTOR_DELAY_TIME_MS - MOTOR_UP_TIME_MS));
        } else {
            k_sleep(K_MSEC(100));
        }
    }
}

void sm_init()
{
    k_mutex_init(&state_mutex);

    /* Outputs configuration */
    gpio_pin_configure_dt(&spot_out, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&strap_out, GPIO_OUTPUT_INACTIVE);

    /* Motor thread configuration */
    atomic_set(&motor_thread_status, 0);
    k_thread_create(thread_get_motor_ctrl(), motor_ctrl_stack, STACK_SIZE, sm_motor_control, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

    k_sleep(K_MSEC(2000));
}

curing_state_t sm_get_state()
{
    k_mutex_lock(&state_mutex, K_FOREVER);
    curing_state_t state = current_state;
    k_mutex_unlock(&state_mutex);
    return state;
}

void sm_set_state(curing_state_t new_state)
{
    switch (new_state) {
    case time_selection:
        /* Deactivate outputs */
        gpio_pin_set_dt(&spot_out, 0);
        gpio_pin_set_dt(&strap_out, 0);
        atomic_set(&motor_thread_status, 0);
        break;
    case curing:
        /* Activate outputs */
        gpio_pin_set_dt(&spot_out, 1);
        gpio_pin_set_dt(&strap_out, 1);
        atomic_set(&motor_thread_status, 1);
        break;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    current_state = new_state;
    k_mutex_unlock(&state_mutex);
}