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


static void init_gpios(void)
{
	int ret = 0,i,target;
	
	// init gpios, in this task button_gpio will not be initialized.
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

static void turnOnLEDs(void)
{
	// turn all LEDs on.
	int i,target;
	
	for(i=0;i<NUM_LED;i++)
	{
		target = LED0_GPIO + i;
		
		gpio_set_value(target,1);
	}
}

static void turnOffLEDS(void)
{
	// turn all LEDs off.
	int i,target;
	
	for(i=0;i<NUM_LED;i++)
	{
		target = LED0_GPIO + i;
		
		gpio_set_value(target,0);
	}
}

static void free_gpios(void)
{
	// release LEDS gpios
	int target,i;
	
	for(i=0;i<NUM_LED;i++)
	{
		target = LED0_GPIO + i;
		gpio_free(target);
	}
}

static int __init bb_module_init(void)
{	
	printk("[EE516] Initializing task2_1 completed.\n");
	// init gpios, then turnOnLEDs
	init_gpios();
	turnOnLEDs();
	return 0;
}

static void __exit bb_module_exit(void)
{		
	printk("[EE516] task2_1 exit.\n");
	
	// turn off led, then release gpios
	turnOffLEDS();
	free_gpios();
}


MODULE_LICENSE("GPL");
module_init(bb_module_init);
module_exit(bb_module_exit);
