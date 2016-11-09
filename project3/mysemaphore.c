#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/compiler.h>
#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/ftrace.h>
#include <linux/list.h>
#include <linux/list_sort.h>

// modes
#define FIFO	0
#define OS		1
#define	USER	2

// maximum values
#define MAX_SEMAPHORE	10
#define MAX_PRIORITY	100

// state
#define ACTIVE		0x10
#define	INACTIVE	0x11

asmlinkage	int	sys_mysema_init(int sema_id, int start_value, int mode);
asmlinkage	int	sys_mysema_down(int sema_id);
asmlinkage	int sys_mysema_down_userprio(int sema_id, int priority);
asmlinkage	int	sys_mysema_up(int sema_id);
asmlinkage	int sys_mysema_release(int sema_id);

struct mysemaphore
{
	raw_spinlock_t		lock;
	int 				mode;
	unsigned int		count;
	int					state;
	struct list_head	wait_list;
};

struct mysemaphore_waiter
{
	struct list_head list;
	struct task_struct *task;
	int priority;
	int up;
};

struct mysemaphore sem_set[MAX_SEMAPHORE];

asmlinkage	int	sys_mysema_init(int sema_id, int start_value, int mode)
{
	static struct lock_class_key __key;
	
	if(0 > sema_id || sema_id >= MAX_SEMAPHORE)
	{
		//error
		printk("mysema_init : invalid semaphore id\n");
		return -1;
	}
	if(sem_set[sema_id].state == ACTIVE)
	{
		//target semaphore is already active
		printk("mysema_init : already active\n");
		return -1;
	}
	if(start_value < 0)
	{
		// start value should be non-negative
		printk("mysema_init : start_value is non-negative\n");
		return -1;
	}
	// initialize target semaphore
	sem_set[sema_id].mode = mode;
	sem_set[sema_id].count = start_value;
	sem_set[sema_id].state = ACTIVE;
	
	INIT_LIST_HEAD(&sem_set[sema_id].wait_list);
	sem_set[sema_id].lock = __RAW_SPIN_LOCK_UNLOCKED(sem_set[sema_id].lock);
	lockdep_init_map(&sem_set[sema_id].lock.dep_map, "semaphore->lock", &__key, 0);
	return 0;
}

asmlinkage	int sys_mysema_down_userprio(int sema_id, int priority)
{
	unsigned long flags;
	struct task_struct *task;
	struct mysemaphore_waiter waiter;
	int target_priority;
	
	if(0 > sema_id || sema_id >= MAX_SEMAPHORE)
	{
		// invalid semaphore id
		printk("mysema_down_userprio : invalid semaphore id\n");
		return -1;
	}
	if(sem_set[sema_id].state == INACTIVE)
	{
		// semaphore that is not initialized
		printk("mysema_down_userprio : inactive semaphore\n");
		return -1;
	}
	if(sem_set[sema_id].mode == OS || sem_set[sema_id].mode == FIFO)
	{
		// above modes are not compatible to this function
		return sys_mysema_down(sema_id);
	}
	if(priority < 0)
	{
		target_priority = 0;
	}
	else if(priority > MAX_PRIORITY)
	{
		target_priority = MAX_PRIORITY;
	}
	else
	{
		target_priority = priority;
	}
	
	raw_spin_lock_irqsave(&sem_set[sema_id].lock, flags);
	if (sem_set[sema_id].count > 0)
	{
		sem_set[sema_id].count--;
	}	
	else
	{
		task = current;

		list_add_tail(&waiter.list, &sem_set[sema_id].wait_list);
		
		waiter.task = task;
		waiter.up = 0;
		waiter.priority = target_priority;
		
		for (;;) 
		{
			__set_task_state(task, TASK_UNINTERRUPTIBLE);
			raw_spin_unlock_irq(&sem_set[sema_id].lock);
			schedule_timeout(MAX_SCHEDULE_TIMEOUT);
			raw_spin_lock_irq(&sem_set[sema_id].lock);
			if (waiter.up == 1)
			{
				break;
			}
		}
	}
	raw_spin_unlock_irqrestore(&sem_set[sema_id].lock, flags);
	
	return 0;
	
}


asmlinkage	int	sys_mysema_down(int sema_id)
{
	unsigned long flags;
	struct task_struct *task;
	struct mysemaphore_waiter waiter;
	
	if(0 > sema_id || sema_id >= MAX_SEMAPHORE)
	{
		// invalid semaphore id
		printk("mysema_down : invalid semaphore id\n");
		return -1;
	}
	if(sem_set[sema_id].state == INACTIVE)
	{
		// semaphore that is not initialized
		printk("mysema_down : inactive semaphore\n");
		return -1;
	}
	
	if(sem_set[sema_id].mode == USER)
	{
		return sys_mysema_down_userprio(sema_id,MAX_PRIORITY);
	}
	
	raw_spin_lock_irqsave(&sem_set[sema_id].lock, flags);
	if (sem_set[sema_id].count > 0)
	{
		sem_set[sema_id].count--;
	}	
	else
	{
		task = current;
		
		list_add_tail(&waiter.list, &sem_set[sema_id].wait_list);
		
		waiter.task = task;
		waiter.up = 0;
		
		for (;;) 
		{
			__set_task_state(task, TASK_UNINTERRUPTIBLE);
			raw_spin_unlock_irq(&sem_set[sema_id].lock);
			schedule_timeout(MAX_SCHEDULE_TIMEOUT);
			raw_spin_lock_irq(&sem_set[sema_id].lock);
			if (waiter.up == 1)
			{
				break;
			}
		}
	}
	raw_spin_unlock_irqrestore(&sem_set[sema_id].lock, flags);
	
	return 0;
}


static int OS_mode_cmp(void *priv, struct list_head *_a, struct list_head *_b)
{
	struct mysemaphore_waiter *a = container_of(_a, struct mysemaphore_waiter, list);
	struct mysemaphore_waiter *b = container_of(_b, struct mysemaphore_waiter, list);
	int priorityA, priorityB;

	priorityA = a->task->prio;
	priorityB = b->task->prio;
	
	if (priorityA < priorityB)
		return -1;
	else if (priorityA > priorityB)
		return 1;
	return 0;
}

static int USER_mode_cmp(void *priv, struct list_head *_a, struct list_head *_b)
{
	struct mysemaphore_waiter *a = container_of(_a, struct mysemaphore_waiter, list);
	struct mysemaphore_waiter *b = container_of(_b, struct mysemaphore_waiter, list);
	int priorityA, priorityB;

	priorityA = a->priority;
	priorityB = b->priority;
	
	if (priorityA < priorityB)
		return -1;
	else if (priorityA > priorityB)
		return 1;
	return 0;
}

asmlinkage	int	sys_mysema_up(int sema_id)
{
	unsigned long flags;
	struct mysemaphore_waiter* targetToWakeup;
	int ret = 0;
	
	if(0 > sema_id || sema_id >= MAX_SEMAPHORE)
	{
		// invalid semaphore id
		printk("mysema_up : invalid semaphore id\n");
		return -1;
	}
	if(sem_set[sema_id].state == INACTIVE)
	{
		// semaphore that is not initialized
		printk("mysema_up : inactive semaphore\n");
		return -1;
	}
	
	raw_spin_lock_irqsave(&sem_set[sema_id].lock, flags);
	if (list_empty(&sem_set[sema_id].wait_list))
	{
		sem_set[sema_id].count++;
	}
	else
	{
		if(sem_set[sema_id].mode == OS)
		{
			// sorting
			list_sort(NULL,&sem_set[sema_id].wait_list,OS_mode_cmp);
			
			targetToWakeup = list_first_entry(&sem_set[sema_id].wait_list,struct mysemaphore_waiter,list);
		}
		else if(sem_set[sema_id].mode == USER)
		{
			// sorting
			list_sort(NULL,&sem_set[sema_id].wait_list,USER_mode_cmp);
			
			targetToWakeup = list_first_entry(&sem_set[sema_id].wait_list,struct mysemaphore_waiter,list);
		}else if(sem_set[sema_id].mode == FIFO)
		{
			// just pick first one
			
			targetToWakeup = list_first_entry(&sem_set[sema_id].wait_list,struct mysemaphore_waiter,list);
		}
		else
		{
			ret = -1;
			printk("mysema_up : invalid mode\n");
			goto error;
		}
		// wakeup
		list_del(&targetToWakeup->list);
		targetToWakeup->up = 1;
		wake_up_process(targetToWakeup->task);
	}
error:
	raw_spin_unlock_irqrestore(&sem_set[sema_id].lock, flags);
	
	return ret;
}

asmlinkage	int sys_mysema_release(int sema_id)
{
	if(0 > sema_id || sema_id >= MAX_SEMAPHORE)
	{
		// invalid semaphore id
		printk("mysema_release : invalid semaphore id\n");
		return -1;
	}
	if(sem_set[sema_id].state == INACTIVE)
	{
		// semaphore that is not initialized
		printk("mysema_release : semaphore is not initialized\n");
		return -1;
	}
	if (!list_empty(&sem_set[sema_id].wait_list))
	{
		// something in the wait queue
		printk("mysema_release : wait queue is not empty\n");
		return -1;
	}
	sem_set[sema_id].state = INACTIVE;
	
	return 0;
}
