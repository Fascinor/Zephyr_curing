#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "state_machine.h"

/* Outputs */
static const struct gpio_dt_spec spot_out =
    GPIO_DT_SPEC_GET(DT_ALIAS(spot_out), gpios);

static const struct gpio_dt_spec strap_out =
    GPIO_DT_SPEC_GET(DT_ALIAS(strap_out), gpios);

static const struct gpio_dt_spec motor_out =
    GPIO_DT_SPEC_GET(DT_ALIAS(motor_out), gpios);

static curing_state_t current_state = time_selection;
static struct k_mutex state_mutex;

void sm_init()
{
    k_mutex_init(&state_mutex);

    /* Outputs configuration */
    gpio_pin_configure_dt(&spot_out, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&strap_out, GPIO_OUTPUT_INACTIVE);

    gpio_pin_configure_dt(&motor_out, GPIO_OUTPUT_INACTIVE);
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
        gpio_pin_set_dt(&motor_out, 0);
        break;
    case curing:
        /* Activate outputs */
        gpio_pin_set_dt(&spot_out, 1);
        gpio_pin_set_dt(&strap_out, 1);
        gpio_pin_set_dt(&motor_out, 1);
        break;
    }
    k_mutex_lock(&state_mutex, K_FOREVER);
    current_state = new_state;
    k_mutex_unlock(&state_mutex);
}