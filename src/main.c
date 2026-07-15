#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

/* Display includes */
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

/* Thread defines */
#define STACK_SIZE 1024
#define THREAD_PRIORITY 5
K_THREAD_STACK_DEFINE(my_stack, STACK_SIZE);
struct k_thread my_thread;

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

/* Inputs */
static const struct gpio_dt_spec up_button =
	GPIO_DT_SPEC_GET(DT_ALIAS(up_button), gpios);

static const struct gpio_dt_spec down_button =
	GPIO_DT_SPEC_GET(DT_ALIAS(down_button), gpios);

static const struct gpio_dt_spec ok_button =
	GPIO_DT_SPEC_GET(DT_ALIAS(ok_button), gpios);

/* Callbacks GPIO */
static struct gpio_callback up_button_cb;
static struct gpio_callback down_button_cb;
static struct gpio_callback ok_button_cb;

/* State machine */
typedef enum {
    time_selection = 0,
    curing
} curing_state_t;

static curing_state_t current_state = time_selection;

/* Curing time */
static uint16_t program_time = 60;
struct k_mutex time_mutex;

/*
 * up_button callback function
 */
void up_button_pressed(const struct device *dev,
		     struct gpio_callback *cb,
		     uint32_t pins)
{
	printk("Up button pressed\n");

	gpio_pin_set_dt(&spot_out, 1);
	gpio_pin_set_dt(&strap_out, 0);
	gpio_pin_set_dt(&motor_out, 0);
}

/*
 * down_button callback function
 */
void down_button_pressed(const struct device *dev,
		     struct gpio_callback *cb,
		     uint32_t pins)
{
	printk("Down button pressed\n");

	gpio_pin_set_dt(&spot_out, 0);
	gpio_pin_set_dt(&strap_out, 1);
	gpio_pin_set_dt(&motor_out, 0);
}

/*
 * ok_button callback function
 */
void ok_button_pressed(const struct device *dev,
		     struct gpio_callback *cb,
		     uint32_t pins)
{
	printk("Ok button pressed\n");

	gpio_pin_set_dt(&spot_out, 0);
	gpio_pin_set_dt(&strap_out, 0);
	gpio_pin_set_dt(&motor_out, 1);
}

void ssd1306_thread(void *arg1, void *arg2, void *arg3)
{
	char time_string[16];
	uint16_t last_time = 0;
	uint16_t p_time_read;
	uint16_t time_hour = 0;
	uint8_t time_min = 0;
	while(true) {
		k_mutex_lock(&time_mutex, K_FOREVER);
		p_time_read = program_time;
		k_mutex_unlock(&time_mutex);
		if (last_time != p_time_read) {
			last_time = p_time_read;
			time_min = last_time%60;
			time_hour = (last_time - time_min)/60;

			cfb_framebuffer_clear(display, true);
			if(current_state == time_selection) {
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

		k_sleep(K_MSEC(100));
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

    ret = gpio_pin_configure_dt(&up_button, GPIO_INPUT);

    ret = gpio_pin_configure_dt(&down_button, GPIO_INPUT);

    ret = gpio_pin_configure_dt(&ok_button, GPIO_INPUT);

	ret = gpio_pin_interrupt_configure_dt(
		&up_button,
		GPIO_INT_EDGE_TO_ACTIVE
	);

	ret = gpio_pin_interrupt_configure_dt(
		&down_button,
		GPIO_INT_EDGE_TO_ACTIVE
	);

	ret = gpio_pin_interrupt_configure_dt(
		&ok_button,
		GPIO_INT_EDGE_TO_ACTIVE
	);


	/* Button callbacks configuration */

	gpio_init_callback(
		&up_button_cb,
		up_button_pressed,
		BIT(up_button.pin)
	);

	ret = gpio_add_callback(up_button.port, &up_button_cb);

	gpio_init_callback(
		&down_button_cb,
		down_button_pressed,
		BIT(down_button.pin)
	);

    ret = gpio_add_callback(down_button.port, &down_button_cb);

	gpio_init_callback(
		&ok_button_cb,
		ok_button_pressed,
		BIT(ok_button.pin)
	);

    ret = gpio_add_callback(ok_button.port, &ok_button_cb);

	printk("System ready\n");

	while (1) {
        k_sleep(K_SECONDS(10));
    }

	return 0;
}