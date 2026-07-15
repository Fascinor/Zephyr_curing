#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include "state_machine.h"
#include <zephyr/sys/printk.h>

/* Outputs */
static const struct gpio_dt_spec spot_out =
    GPIO_DT_SPEC_GET(DT_ALIAS(spot_out), gpios);

static const struct gpio_dt_spec strap_out =
    GPIO_DT_SPEC_GET(DT_ALIAS(strap_out), gpios);

static const struct pwm_dt_spec motor_pwm =
    PWM_DT_SPEC_GET(DT_ALIAS(motor_out));

static curing_state_t current_state = time_selection;
static struct k_mutex state_mutex;

void sm_init()
{
    k_mutex_init(&state_mutex);

    /* Outputs configuration */
    gpio_pin_configure_dt(&spot_out, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&strap_out, GPIO_OUTPUT_INACTIVE);
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
        pwm_set_dt(&motor_pwm, 20000, 0);
        break;
    case curing:
        /* Activate outputs */
        gpio_pin_set_dt(&spot_out, 1);
        gpio_pin_set_dt(&strap_out, 1);
        pwm_set_dt(&motor_pwm, 20000, 10000);
        break;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    current_state = new_state;
    k_mutex_unlock(&state_mutex);
}