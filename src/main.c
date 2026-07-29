#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

/* Display includes */
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

/* Buttons include */
#include <zephyr/input/input.h>

/* Private includes */
#include "state_machine.h"
#include "curing_time.h"

/* Buttons define */
#define SCREEN_REFRESH_RATE_MS			50
#define TIME_CONTINUOUS_INC_DEC_DELAY	100

/* Thread defines */
#include "thread_fn.h"
K_THREAD_STACK_DEFINE(ssd1306_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(curing_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(time_increase_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(time_decrease_stack, STACK_SIZE);

/* ssd1306 */
static const struct device *display =
    DEVICE_DT_GET(DT_ALIAS(display));

/* Thread functions declarations */
static void ssd1306_thread(void *arg1, void *arg2, void *arg3);
static void curing_thread_fn(void *arg1, void *arg2, void *arg3);
void time_increase_fn(void *arg1, void *arg2, void *arg3);
void time_decrease_fn(void *arg1, void *arg2, void *arg3);

/**
 * @brief Callback for buttons events
 * @param evt event calling the callback.
 * @param user_data not used 
 */
static void input_event_cb(struct input_event *evt, void *user_data)
{
    ARG_UNUSED(user_data);
    static atomic_t increase_thread_status;
    static atomic_t decrease_thread_status;

    switch (evt->code) {

    case INPUT_KEY_F1: // Up short
        if ((evt->value == 1) && sm_get_state() == time_selection) {
            ct_increase_time();
        }
        break;

    case INPUT_KEY_F2: // Down short
        if((evt->value == 1) && sm_get_state() == time_selection) {
            ct_decrease_time();
        }
        break;

    case INPUT_KEY_F3: // Enter short
        if((evt->value == 1) && sm_get_state() == time_selection) {
            k_thread_create(thread_get_curing(), curing_stack, STACK_SIZE, curing_thread_fn, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
        }
        break;

    case INPUT_KEY_F4: // Up long
        if((evt->value == 1) && sm_get_state() == time_selection) {
            atomic_set(&increase_thread_status, 1);
            k_thread_create(thread_get_time_increase(), time_increase_stack, STACK_SIZE, time_increase_fn, &increase_thread_status, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
        } else if((evt->value == 0) && sm_get_state() == time_selection) {
            atomic_set(&increase_thread_status, 0);
        }
        break;

    case INPUT_KEY_F5: // Down long
        if((evt->value == 1) && sm_get_state() == time_selection) {
            atomic_set(&decrease_thread_status, 1);
            k_thread_create(thread_get_time_decrease(), time_decrease_stack, STACK_SIZE, time_decrease_fn, &decrease_thread_status, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
        } else if((evt->value == 0) && sm_get_state() == time_selection) {
            atomic_set(&decrease_thread_status, 0);
        }
        break;

    case INPUT_KEY_F6: // Enter long
        if((evt->value == 1) && sm_get_state() == curing) {
            sm_set_state(time_selection);
        }
        break;
    }
}

/**
 * @brief Thread-function that increases the time automatically
 * @param arg1 not used
 * @param arg2 not used 
 * @param arg3 not used 
 */
void time_increase_fn(void *arg1, void *arg2, void *arg3)
{
    atomic_t *increase_thread_status = (atomic_t *) arg1;
    while(atomic_get(increase_thread_status)) {
        if(ct_get_time() < CURING_TIME_MAX_VALUE) {
            ct_increase_time();
        }
        k_sleep(K_MSEC(TIME_CONTINUOUS_INC_DEC_DELAY));
    }
}

/**
 * @brief Thread-function that decreases the time automatically
 * @param arg1 not used
 * @param arg2 not used 
 * @param arg3 not used 
 */
void time_decrease_fn(void *arg1, void *arg2, void *arg3)
{
    atomic_t *decrease_thread_status = (atomic_t *) arg1;
    while(atomic_get(decrease_thread_status)) {
        if(ct_get_time() > 0) {
            ct_decrease_time();
        }
        k_sleep(K_MSEC(TIME_CONTINUOUS_INC_DEC_DELAY));
    }
}

/**
 * @brief Thread-function that decreases the time every minute while in curing state
 * @param arg1 not used
 * @param arg2 not used 
 * @param arg3 not used 
 */
void curing_thread_fn(void *arg1, void *arg2, void *arg3)
{
    bool running = true;
    uint16_t last_time;
    sm_set_state(curing);

    while(running) {
        last_time = ct_get_time();
        if((last_time <= 0) || sm_get_state() != curing) {
            running = false;
        } else {
            k_sleep(K_SECONDS(60)); // 1min
            ct_set_time(last_time - 1);
        }
    }

    if(ct_get_time() == 0) {
        ct_set_time(CURING_TIME_DEFAULT_VALUE);
    }
    sm_set_state(time_selection);
}

/**
 * @brief Thread-function. Check every SCREEN_REFRESH_RATE_MS (50ms) to see if an info changed and refresh the screen if needed
 * @param arg1 not used
 * @param arg2 not used 
 * @param arg3 not used 
 */
void ssd1306_thread(void *arg1, void *arg2, void *arg3)
{
    char time_string[16];
    curing_state_t last_state = time_selection;
    uint16_t last_time = 0;
    uint16_t p_time_read;
    uint16_t time_hour = 0;
    uint8_t time_min = 0;
    while(true) {
        p_time_read = ct_get_time();
        if ((last_time != p_time_read) || last_state != sm_get_state()) {
            last_state = sm_get_state();
            last_time = p_time_read;
            time_min = last_time%60;
            time_hour = (last_time - time_min)/60;

            cfb_framebuffer_clear(display, true);
            if(sm_get_state() == time_selection) {
                cfb_print(display, "Select time", 0, 6);
            } else {
                cfb_print(display, "Curing", 0, 6);
            }
            if(last_time < 60) {
                sprintf(time_string, "%dmin", last_time);
            } else {
                sprintf(time_string, "%dh%dmin", time_hour, time_min);
            }
            cfb_print(display, time_string, 32, 19);
            cfb_framebuffer_finalize(display);
        }

        k_sleep(K_MSEC(SCREEN_REFRESH_RATE_MS));
    }
}

int main(void)
{
    /* Mutexes initialisation */
    sm_init();
    ct_mutex_init();

    /* Display configuration */
    if (!device_is_ready(display)) {
        printk("Display not ready\n");
        return 0;
    }
    cfb_framebuffer_init(display);
    cfb_framebuffer_set_font(display, 0);
    k_thread_create(thread_get_ssd1306(), ssd1306_stack, STACK_SIZE, ssd1306_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

    /* Button callback configuration */
    INPUT_CALLBACK_DEFINE(NULL, input_event_cb, NULL);

    printk("System ready\n");

    while (1) {
        k_sleep(K_SECONDS(10));
    }

    return 0;
}