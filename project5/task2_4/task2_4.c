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
	int state = gpio_get_value(BUTTON_GPIO);
	unsigned long long duration;
	
	printk("button state : %d\n",state);
	
	if(state == 0)
	{
		startTime = get_jiffies_64();
		// falling
	}
	else
	{
		// rising
		duration = get_jiffies_64() - startTime;
		
		if(duration <= HZ)
		{
			counter++;
			if(counter == 16)
			{
				counter = 0;
			}
		}
		else
		{
			counter = 0;
		}
	}
	
	return (irq_handler_t)IRQ_HANDLED;
}



static void init_gpios(void)
{
	int ret = 0,i,target;
	
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
	buttonIRQ = gpio_to_irq(BUTTON_GPIO);
	
	ret = request_irq(buttonIRQ,(irq_handler_t)button_isr,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,"EE516_TEAM1",NULL);
	
	if(ret < 0)
	{
		printk("request_irq failure\n");
	}
}

void timer_timeout(unsigned long ptr)
{
	int i,cur,target,j;
	
	if(ptr == LIGHTDOWN)
	{
		// turn on the light
		for(i=1,j=0;i<=8;i*=2,j++)
		{
			cur = counter&i;
			
			if(cur != 0)
			{
				gpio_set_value(LED0_GPIO+j,1);
			}
		}
		timerFactor.expires = get_jiffies_64() + 8*HZ/10; // 0.8secs
		timerFactor.data = LIGHTUP;
	}else if(ptr == LIGHTUP)
	{
		// turn off the light
		for(i=0;i<NUM_LED;i++)
		{
			gpio_set_value(LED0_GPIO+i,0);
		}
		timerFactor.expires = get_jiffies_64() + 2*HZ/10; // 0.2secs
		timerFactor.data = LIGHTDOWN;
	}
	add_timer(&timerFactor);
}

static void start_timer(void)
{
	init_timer(&timerFactor);
	timerFactor.function = timer_timeout;
	timerFactor.expires = get_jiffies_64() + 2*HZ/10; // 0.2secs
	timerFactor.data = LIGHTDOWN;
	
	add_timer(&timerFactor);
}

static void stop_timer(void)
{
	int i;
	
	for(i=0;i<NUM_LED;i++)
	{
		gpio_set_value(LED0_GPIO+i,0);
	}
	del_timer(&timerFactor);
}

static void free_gpios(void)
{
	int target,i;
	
	for(i=0;i<NUM_LED;i++)
	{
		target = LED0_GPIO + i;
		gpio_free(target);
	}
	gpio_free(BUTTON_GPIO);
}

static int __init bb_module_init(void)
{	
	printk("[EE516] Initializing BB module completed.\n");
	init_gpios();
	counter = 0;
	start_timer();
	return 0;
}

static void __exit bb_module_exit(void)
{		
	printk("[EE516] BB module exit.\n");
	free_gpios();
	stop_timer();
	free_irq(buttonIRQ,NULL);
}


MODULE_LICENSE("GPL");
module_init(bb_module_init);
module_exit(bb_module_exit);
