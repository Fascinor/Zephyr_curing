#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

/* Display includes */
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

/* Buttons include */
#include <zephyr/input/input.h>

/* Buttons define */
#define SCREEN_REFRESH_RATE_MS			50
#define TIME_CONTINUOUS_INC_DEC_DELAY	100
#define CURING_TIME_MAX_VALUE			3000
#define CURING_TIME_DEFAULT_VALUE		60
#define THRESHOLD_1_TO_5				60

/* Thread defines */
#define STACK_SIZE 1024
#define THREAD_PRIORITY 5
K_THREAD_STACK_DEFINE(my_stack, STACK_SIZE);
struct k_thread my_thread;
K_THREAD_STACK_DEFINE(curing_stack, STACK_SIZE);
struct k_thread curing_thread;
K_THREAD_STACK_DEFINE(time_increase_stack, STACK_SIZE);
struct k_thread time_increase_thread;
K_THREAD_STACK_DEFINE(time_decrease_stack, STACK_SIZE);
struct k_thread time_decrease_thread;

/* ssd1306 */
static const struct device *display =
    DEVICE_DT_GET(DT_ALIAS(display));

/* Outputs */
static const struct gpio_dt_spec spot_out =
	GPIO_DT_SPEC_GET(DT_ALIAS(spot_out), gpios);

static const struct gpio_dt_spec strap_out =
	GPIO_DT_SPEC_GET(DT_ALIAS(strap_out), gpios);

static const struct gpio_dt_spec motor_out =
	GPIO_DT_SPEC_GET(DT_ALIAS(motor_out), gpios);

/* State machine */
typedef enum {
    time_selection = 0,
    curing
} curing_state_t;

static curing_state_t current_state = time_selection;
struct k_mutex state_mutex;

static curing_state_t get_state()
{
	k_mutex_lock(&state_mutex, K_FOREVER);
	curing_state_t state = current_state;
	k_mutex_unlock(&state_mutex);
	return state;
}

static void set_state(curing_state_t new_state)
{
  switch (new_state) {
  case time_selection:
    /* Deactivate outputs */
    gpio_pin_set_dt(&spot_out, 0);
    gpio_pin_set_dt(&strap_out, 0);
    gpio_pin_set_dt(&motor_out, 0);
    printk("In time_selection state\n");
    break;
  case curing:
    /* Activate outputs */
    gpio_pin_set_dt(&spot_out, 1);
    gpio_pin_set_dt(&strap_out, 1);
    gpio_pin_set_dt(&motor_out, 1);
    printk("In curing state\n");
    break;
  }
  k_mutex_lock(&state_mutex, K_FOREVER);
  current_state = new_state;
  k_mutex_unlock(&state_mutex);
}

/* Thread functions declarations */
static void ssd1306_thread(void *arg1, void *arg2, void *arg3);
static void curing_thread_fn(void *arg1, void *arg2, void *arg3);
void time_increase_fn(void *arg1, void *arg2, void *arg3);
void time_decrease_fn(void *arg1, void *arg2, void *arg3);

/* Curing time */
static uint16_t program_time = CURING_TIME_DEFAULT_VALUE;
struct k_mutex time_mutex;

static void increase_time()
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

static void decrease_time()
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
	}else {
		program_time = 0;
	}
	
	k_mutex_unlock(&time_mutex);
}

static void input_event_cb(struct input_event *evt, void *user_data)
{
    ARG_UNUSED(user_data);
	static atomic_t increase_thread_status;
	static atomic_t decrease_thread_status;

    //printk("code=%u value=%d\n", evt->code, evt->value);

    switch (evt->code) {

    case INPUT_KEY_F1: // Up short
		if ((evt->value == 1) && get_state() == time_selection) {
			increase_time();
		}
    	break;

	case INPUT_KEY_F2: // Down short
		if((evt->value == 1) && get_state() == time_selection) {
	    	decrease_time();
		}
	    break;

	case INPUT_KEY_F3: // Enter short
		if((evt->value == 1) && get_state() == time_selection) {
			k_thread_create(&curing_thread, curing_stack, STACK_SIZE, curing_thread_fn, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
		}
    	break;

	case INPUT_KEY_F4: // Up long
		if((evt->value == 1) && get_state() == time_selection) {
			atomic_set(&increase_thread_status, 1);
			k_thread_create(&time_increase_thread, time_increase_stack, STACK_SIZE, time_increase_fn, &increase_thread_status, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
		} else if((evt->value == 0) && get_state() == time_selection) {
			atomic_set(&increase_thread_status, 0);
		}
	    break;

	case INPUT_KEY_F5: // Down long
		if((evt->value == 1) && get_state() == time_selection) {
			atomic_set(&decrease_thread_status, 1);
			k_thread_create(&time_decrease_thread, time_decrease_stack, STACK_SIZE, time_decrease_fn, &decrease_thread_status, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
		} else if((evt->value == 0) && get_state() == time_selection) {
			atomic_set(&decrease_thread_status, 0);
		}
	    break;

	case INPUT_KEY_F6: // Enter long
		if((evt->value == 1) && get_state() == curing) {
			set_state(time_selection);
		}
	    break;
    }
}

void time_increase_fn(void *arg1, void *arg2, void *arg3)
{
	atomic_t *increase_thread_status = (atomic_t *) arg1;
	while(atomic_get(increase_thread_status)) {
		if(program_time < CURING_TIME_MAX_VALUE) {
			increase_time();
		}
		k_sleep(K_MSEC(TIME_CONTINUOUS_INC_DEC_DELAY));
	}
}

void time_decrease_fn(void *arg1, void *arg2, void *arg3)
{
	atomic_t *decrease_thread_status = (atomic_t *) arg1;
	while(atomic_get(decrease_thread_status)) {
		if(program_time > 0) {
			decrease_time();
		}
		k_sleep(K_MSEC(TIME_CONTINUOUS_INC_DEC_DELAY));
	}
}

void curing_thread_fn(void *arg1, void *arg2, void *arg3)
{
	bool running = true;
	set_state(curing);

	while(running) {
		k_sleep(K_SECONDS(60));
		k_mutex_lock(&time_mutex, K_FOREVER);
		if((program_time <= 0) || get_state() != curing) {
			running = false;
		} else {
			program_time--;
		}
		k_mutex_unlock(&time_mutex);
	}

	if(program_time == 0) {
		program_time = CURING_TIME_DEFAULT_VALUE;
	}
	set_state(time_selection);
}

void ssd1306_thread(void *arg1, void *arg2, void *arg3)
{
	char time_string[16];
	curing_state_t last_state = time_selection;
	uint16_t last_time = 0;
	uint16_t p_time_read;
	uint16_t time_hour = 0;
	uint8_t time_min = 0;
	while(true) {
		k_mutex_lock(&time_mutex, K_FOREVER);
		p_time_read = program_time;
		k_mutex_unlock(&time_mutex);
		if ((last_time != p_time_read) || last_state != get_state()) {
			last_state = get_state();
			last_time = p_time_read;
			time_min = last_time%60;
			time_hour = (last_time - time_min)/60;

			cfb_framebuffer_clear(display, true);
			if(get_state() == time_selection) {
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
	int ret;

	k_mutex_init(&time_mutex);

	/* Display configuration */

    if (!device_is_ready(display)) {
    	printk("Display not ready\n");
    	return 0;
	}

	ret = cfb_framebuffer_init(display);

	if (ret) {
    	printk("CFB init failed (%d)\n", ret);
    	return 0;
	}

	cfb_framebuffer_set_font(display, 0);

	k_thread_create(&my_thread, my_stack, STACK_SIZE, ssd1306_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

	/* Outputs configuration */

	ret = gpio_pin_configure_dt(&spot_out, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		printk("Erreur spot_out\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&strap_out, GPIO_OUTPUT_INACTIVE);
	if (ret) { 
		printk("Erreur strap_out\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&motor_out, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		printk("Erreur motor_out\n");
		return 0;
	}


	/* Button configuration */

    INPUT_CALLBACK_DEFINE(NULL, input_event_cb, NULL);

	printk("System ready\n");

	while (1) {
        k_sleep(K_SECONDS(10));
    }

	return 0;
}