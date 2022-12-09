
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec pwm_led2 = PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});

#define NUM_STEPS	50U
#define SLEEP_MSEC	25U

K_SEM_DEFINE(pb_pushed, 0, 1);

bool led_enabled;

static struct gpio_callback button_cb_data;

static struct k_work led_indicator_work;
static void led_indicator_work_function(struct k_work *work);

void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_sem_give(&pb_pushed);
}

void main(void)
{
	int ret;
	led_enabled = true;

	printk("PWM-based RGB LED fade\n");

	if (!device_is_ready(pwm_led0.dev)) {
		printk("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return;
	}

	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_callback, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	k_work_init(&led_indicator_work, led_indicator_work_function);
	k_work_submit(&led_indicator_work);

	while (1) {

		if (k_sem_take(&pb_pushed, K_FOREVER) != 0) {
			printk("Error\n");
		} else {
			printk("Push button pressed\n");
			do {
				k_sleep(K_MSEC(100));
			} while(gpio_pin_get_dt(&button));
			printk("Push button released\n");
			led_enabled = !led_enabled;
			if (led_enabled) printk("LED is enabled\n");
			else 			 printk("LED is disabled\n");
		}
	}
}

static void led_indicator_work_function(struct k_work *work)
{
	// Turn off LEDs
	pwm_set_pulse_dt(&pwm_led0, 0);
	pwm_set_pulse_dt(&pwm_led1, 0);
	pwm_set_pulse_dt(&pwm_led2, 0);

	while(1) {
			fade_pwm_led(&pwm_led0);
			fade_pwm_led(&pwm_led1);
			fade_pwm_led(&pwm_led2);
	}
}

void fade_pwm_led(const struct pwm_dt_spec *spec)
{
	uint32_t pulse_width = 0U;
	uint32_t step = pwm_led0.period / NUM_STEPS;
	uint8_t dir = 1U;
	int ret;

	while (1) {
		ret = pwm_set_pulse_dt(spec, pulse_width);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}

		if (dir) {
			pulse_width += step;
			if (pulse_width >= pwm_led0.period) {
				pulse_width = pwm_led0.period - step;
				dir = 0U;
			}
		} else {
			if (pulse_width >= step) {
				pulse_width -= step;
			} else {
				pulse_width = step;
				dir = 1U;
				break;
			}
		}

		// break loop if button pressed
		//if (!led_enabled) break;	

		k_sleep(K_MSEC(SLEEP_MSEC));
	}

	// Disable LED
	pwm_set_pulse_dt(spec, 0);
} 
