#include <zephyr/kernel.h>

#define STACK 2048
K_THREAD_STACK_DEFINE(stk, STACK);
static struct k_thread t;

static void fn(void *a, void *b, void *c)
{
	for (;;) {
		printk("thread on cpu %d\n", arch_curr_cpu()->id);
		k_busy_wait(500 * 1000);
		//k_msleep(500);
	}
}

int main(void)
{
	k_tid_t id = k_thread_create(&t, stk, STACK, fn, NULL, NULL, NULL,
				     5, 0, K_FOREVER);
	k_thread_cpu_pin(id, 1);
	k_thread_start(id);
	printk("main on cpu %d\n", arch_curr_cpu()->id);
	return 0;
}
