#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

/* Display includes */
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

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

void ssd_print(char *buffer, int posx, int posy)
{
	cfb_framebuffer_clear(display, true);

	cfb_print(display, buffer, posx, posy + 8);

	cfb_framebuffer_finalize(display);
}


int main(void)
{
	int ret;

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

	ssd_print("hello there", 0, 0);

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