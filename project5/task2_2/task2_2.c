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

struct timer_list timerFactor[NUM_LED];


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
	
}

void timer_timeout(unsigned long ptr)
{
	int i,cur,target;
	
	
	if(0<= ptr && ptr < 4)
	{
		cur = gpio_get_value(LED0_GPIO+ptr);
		target = LED0_GPIO + ptr;
		
		if(cur==0)
		{
			gpio_set_value(target,1);
		}
		else
		{
			gpio_set_value(target,0);
		}
		timerFactor[ptr].expires = get_jiffies_64() + (ptr+1)*HZ / 10;
	
		add_timer(&timerFactor[ptr]);
	}
}

static void start_timer(void)
{
	int i;
	
	for(i=0;i<NUM_LED;i++)
	{
		init_timer(&timerFactor[i]);
		timerFactor[i].function = timer_timeout;
		timerFactor[i].expires = get_jiffies_64() + (i+1)*HZ / 10;
		timerFactor[i].data = i;
		add_timer(&timerFactor[i]);
	}
}

static void stop_timer(void)
{
	int i;
	
	for(i=0;i<NUM_LED;i++)
	{
		del_timer(&timerFactor[i]);
	}
}

static void free_gpios(void)
{
	int target,i;
	
	for(i=0;i<NUM_LED;i++)
	{
		target = LED0_GPIO + i;
		gpio_free(target);
	}
}

static int __init bb_module_init(void)
{	
	printk("[EE516] Initializing BB module completed.\n");
	init_gpios();
	start_timer();
	return 0;
}

static void __exit bb_module_exit(void)
{		
	printk("[EE516] BB module exit.\n");
	stop_timer();
	free_gpios();
}


MODULE_LICENSE("GPL");
module_init(bb_module_init);
module_exit(bb_module_exit);
