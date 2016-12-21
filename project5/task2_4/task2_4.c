/*
   	LED and Button Driver for Beagle Bone Black
	Copyright (C) 1984-2015 Core lab. <djshin.core.kaist.ac.kr>

*/

#include <linux/module.h>
#include <linux/kernel.h> /* printk */
#include <linux/interrupt.h> /* irq_request */
#include <linux/irq.h>
#include <asm/uaccess.h> /* copy_to_user */
#include <linux/fs.h> /* file_operations */
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/delay.h>   /* msleep */
#include <linux/timer.h>   /* kernel timer */
#include <linux/slab.h>	   /* kmalloc */


#define NUM_LED 4
#define TIME_STEP  (1*HZ)

#define LED0_GPIO   53    /* USER LED 0     */
#define LED1_GPIO   54    /* USER LED 1     */
#define LED2_GPIO   55    /* USER LED 2     */
#define LED3_GPIO   56    /* USER LED 3     */
#define BUTTON_GPIO 72    /* USER BUTTON S2 */

#define LIGHTUP	0x06
#define LIGHTDOWN 0x07

struct timer_list timerFactor;

int buttonIRQ;
int timerActivated;
int counter;
unsigned long long startTime;

static void start_timer(void);
static void stop_timer(void);


static irq_handler_t button_isr(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
	/*
	 * button's irq handler, it will be invoked by kernel when physical button is pushed(falling) and detached(rising)
	 * */
	int state = gpio_get_value(BUTTON_GPIO); // retrieve the state of button's gpio. it can be used to classify the falling or rising.
	unsigned long long duration;
	
	printk("button state : %d\n",state);
	
	/*
	 * for calculating the duration of button down, we used the jiffies from kernel*/
	if(state == 0)
	{
		// falling
		startTime = get_jiffies_64();
	}
	else
	{
		// rising
		duration = get_jiffies_64() - startTime;
		
		if(duration <= HZ)
		{
			// duration is under the 1000ms
			counter++;
			// because there are 4 leds in bbb, it can't represent the value above 16. So revert it to zero
			if(counter == INT_MAX)
			{
				counter = 0;
			}
		}
		else
		{
			// long click is detected. set counter variable to zero.
			printk("long click! reset counter.\n");
			counter = 0;
		}
		printk("counter : %d\n", counter);
	}
	
	return (irq_handler_t)IRQ_HANDLED;
}



static void init_gpios(void)
{
	int ret = 0,i,target;
	
	// init gpios, including button_gpio.
	for(i=0;i<NUM_LED;i++)
	{
		target = LED0_GPIO + i;
		ret = gpio_request(target,"LED");
	
		if(ret < 0)
		{
			printk("LED%d gpio_init failure\n",i);
		}
		gpio_direction_output(target,0);
	}
	ret = gpio_request(BUTTON_GPIO,"BUTTON");
	if(ret < 0)
	{
		printk("Button gpio gpio_init failure\n");
	}
	buttonIRQ = gpio_to_irq(BUTTON_GPIO);
	
	// In task2_4, button should be activated. So acquire the irq number from gpio then register irq handler with FALLING and RISING edge
	// In this task, Detecting falling and rising edge is necessary to control the timer.
	ret = request_irq(buttonIRQ,(irq_handler_t)button_isr,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,"EE516_TEAM1",NULL);
	
	if(ret < 0)
	{
		printk("request_irq failure\n");
	}
}

void timer_timeout(unsigned long ptr)
{
	int i,cur;
	
	/*
	 * timer expired handler for blinking leds by referring current counter value.
	 * */
	
	// ptr represent previous state of leds
	if(ptr == LIGHTDOWN)
	{
		// turn on the light by referring counter value. 
		for(i=0;i<NUM_LED;i++)
		{
			// AND operation for extracting target bit is 1 or 0.
			cur = counter&(1<<i);
			
			if(cur != 0)
			{
				// target bit is 1.
				gpio_set_value(LED0_GPIO+i,1);
			}
		}
		// light up for 0.8 secs
		timerFactor.expires = get_jiffies_64() + 8*HZ/10; // 0.8secs
		timerFactor.data = LIGHTUP;
	}else if(ptr == LIGHTUP)
	{
		// turn off the light
		for(i=0;i<NUM_LED;i++)
		{
			gpio_set_value(LED0_GPIO+i,0);
		}
		// light down for 0.2 secs
		timerFactor.expires = get_jiffies_64() + 2*HZ/10; // 0.2secs
		timerFactor.data = LIGHTDOWN;
	}
	add_timer(&timerFactor);
}

static void start_timer(void)
{
	init_timer(&timerFactor);
	// start with LIGHTDOWN state.
	timerFactor.function = timer_timeout;
	timerFactor.expires = get_jiffies_64() + 2*HZ/10; // 0.2secs
	timerFactor.data = LIGHTDOWN;
	
	add_timer(&timerFactor);
}

static void stop_timer(void)
{
	int i;
	
	// turn off LEDs and remove timer from kernel
	for(i=0;i<NUM_LED;i++)
	{
		gpio_set_value(LED0_GPIO+i,0);
	}
	del_timer(&timerFactor);
}

static void free_gpios(void)
{
	int target,i;
	
	// release gpios including button's gpio
	for(i=0;i<NUM_LED;i++)
	{
		target = LED0_GPIO + i;
		gpio_free(target);
	}
	free_irq(buttonIRQ,NULL);
	gpio_free(BUTTON_GPIO);
}

static int __init bb_module_init(void)
{	
	printk("[EE516] Initializing task2_4 completed.\n");
	
	// init gpios then start timer
	init_gpios();
	counter = 0;
	start_timer();
	return 0;
}

static void __exit bb_module_exit(void)
{		
	printk("[EE516] task2_4 exit.\n");
	// stop timer then free gpios
	stop_timer();
	free_gpios();
}


MODULE_LICENSE("GPL");
module_init(bb_module_init);
module_exit(bb_module_exit);
