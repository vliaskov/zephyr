/*
 * Repro for zephyrproject-rtos/zephyr#123657 (fix proposed in PR #123658):
 * LEDC low-speed timers share one clock source; configuring one timer can
 * switch that clock and change the frequency of another running timer.
 * We use LEDs and lower frequencies than the issue-described reproducer
 * that uses servos.
 *
 * Wiring: logic analyzer / scope on GPIO4 (ch0) and GPIO5 (ch1).
 * Or visual test by connecting LEDs to the two GPIOs.
 *
 * Expected (correct) behaviour: GPIO4 stays at 1 Hz in every phase.
 * Bug: GPIO4's ch0 frequency/period changes when GPIO5 ch1 is started.
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>

#define LEDC_NODE DT_NODELABEL(ledc0)

/* the slow pin/pulse */
#define CH_SLOW   0   /* GPIO4, timer 0 */
/* the interfering pin/pulse */
#define CH_OTHER   1   /* GPIO5, timer 1 */

#define SLOW_PERIOD   PWM_HZ(1)
#define SLOW_PULSE    SLOW_PERIOD / 2 

static const struct device *const pwm = DEVICE_DT_GET(LEDC_NODE);

static int slow_start(void)
{
	return pwm_set(pwm, CH_SLOW, SLOW_PERIOD, SLOW_PULSE, 0);
}

static int slow_stop(void)
{
	return pwm_set(pwm, CH_SLOW, SLOW_PERIOD, 0, 0);
}

static int other_start(uint32_t hz)
{
	uint32_t period = PWM_HZ(hz);

	return pwm_set(pwm, CH_OTHER, period, period / 2, 0);
}

static int other_stop(void)
{
	return pwm_set(pwm, CH_OTHER, PWM_HZ(1000), 0, 0);
}

static void check(const char *what, int ret)
{
	if (ret) {
		printk("  !! %s failed: %d\n", what, ret);
	}
}

/*
 * Scenario (from the PR): slow LED GPIO4 keeps running while a second timer is
 * configured. GPIO4 blinking frequency should NOT change when GPIO5 starts.
 */
static void scenario_running_slow(uint32_t other_hz)
{
	printk("\n[A] slow(GPIO4) running, then start %u Hz on GPIO5\n", other_hz);

	check("slow_start", slow_start());
	printk("  GPIO4 = 20 Hz. Measure it now (reference)\n");
	k_sleep(K_SECONDS(4));

	check("other_start", other_start(other_hz));
	printk("  GPIO5 = %u Hz started. Is GPIO4 still 1 Hz?\n", other_hz);
	k_sleep(K_SECONDS(4));

	check("other_stop", other_stop());
	check("slow_stop", slow_stop());
	k_sleep(K_SECONDS(1));
}

int main(void)
{
	if (!device_is_ready(pwm)) {
		printk("LEDC device not ready\n");
		return 0;
	}

	printk("LEDC clock-sharing repro (zephyr #123657)\n");

	while (1) {
		/* PR #123658 describes 20 kHz for servos; here we use LEDs, let' say 20 Hz. */
		scenario_running_slow(20);
	}

	return 0;
}
