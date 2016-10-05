#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>

ssize_t hw1_read(struct file *file, const char *buffer, size_t length, loff_t *offset);
ssize_t hw1_write(struct file *file, const char *buffer, size_t length, loff_t *offset);
int hw1_ioctl(struct file *file, unsigned int cmd, unsigned long args);

int modifyStack(int type, int v);

#define HW1_DRIVER_MAJOR_NUMBER 250
#define DRIVER_NAME "JUSTICERAINSFROMABOVE"

#define PUSH 0x1234
#define POP 0x1235
#define CLEAN 0x1236

int stack[257];
int stackpointer = -1;

struct mutex my_mutex;


bool empty(void)
{
	return stackpointer == -1 ? true : false;
}

int size(void)
{
	return stackpointer+1;
}

bool full(void)
{
	return (stackpointer == 255) ? true : false;
}

int push(int v)
{
	int r;
	mutex_lock(&my_mutex);
	r = modifyStack(PUSH,v);
	mutex_unlock(&my_mutex);
	return r;
}

int pop(void)
{
	int r;
	mutex_lock(&my_mutex);
	r = modifyStack(POP,0);
	mutex_unlock(&my_mutex);
	return r;
}

int modifyStack(int type,int v)
{
        int r;
        if(type == PUSH)
        {
                if(!full())
                {
                        stackpointer++;
                        stack[stackpointer] = v;
                        printk("%d is pushed to stack, size is %d\n",v,size());
                        r = size();
                }
                else
                {
                        r = 0x7fffffff;
                }
        }
        else if(type == POP)
        {
                if(!empty())
                {
                        r = stack[stackpointer];
                        stackpointer--;
                        printk("%d is popped from stack, size is %d\n",r,size());
                }
                else
                {
                        r = 0x7fffffff;
                }
        }
        else if(type == CLEAN)
        {
                stackpointer = -1;
                printk("Stack has been cleaned\n");
        }
        else
        {
                r = 0x7fffffff;
        }
        return r;
}

static struct file_operations fops ={
	.read = hw1_read,
	.write = hw1_write,
	.unlocked_ioctl = hw1_ioctl
};

static struct cdev hw1_cdev;

ssize_t hw1_read(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
	printk("hw1_read invoked\n");
	int k = pop();
	if(k == 0x7fffffff)
	{
		return -1;
	}
	return k;
}

ssize_t hw1_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
	printk("hw1_write invoked\n");
	int k = buffer[0];
	int r = push(k);
	return r;
}

int hw1_ioctl(struct file *file, unsigned int cmd, unsigned long args)
{
	switch(cmd)
	{
	case 0:
		printk("hw1_ioctl invoked : clean command\n");
		mutex_lock(&my_mutex);
		modifyStack(CLEAN,0);
		mutex_unlock(&my_mutex);
		break;
	default:
		printk("hw1_ioctl : other commands are not supported\n");
		break;
	}
	return 0;
}

static int __init init(void)
{
	dev_t dev;
	int err;

	mutex_init(&my_mutex);

	dev = MKDEV(HW1_DRIVER_MAJOR_NUMBER,0);
	
	err = register_chrdev_region(dev,1,DRIVER_NAME);

	cdev_init(&hw1_cdev,&fops);

	cdev_add(&hw1_cdev,dev,128);


	printk("hw1_driver : init\n");
	return 0;
}

static void __exit exit(void)
{
	cdev_del(&hw1_cdev);
	printk("hw1_driver : exit\n");
}

module_init(init);
module_exit(exit);

MODULE_AUTHOR("SBH");
MODULE_DESCRIPTION("HW1_DRIVER");
MODULE_LICENSE("GPL");
