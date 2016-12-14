/*
		Dummy Driver for LinkedList
*/

#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/random.h>
#include <linux/slab.h>

#define DUMMY_MAJOR_NUMBER 250
#define DUMMY_DEVICE_NAME "JUSTICERAINSFROM"

int dummy_open(struct inode *, struct file *);
int dummy_release(struct inode *, struct file *);
ssize_t dummy_read(struct file *, char *, size_t, loff_t *);
ssize_t dummy_write(struct file *, const char *, size_t, loff_t *);
long dummy_ioctl(struct file *, unsigned int, unsigned long);


/* file operation structure */
struct file_operations dummy_fops ={
	open: dummy_open,
	read: dummy_read,
	write: dummy_write,
	release: dummy_release, 
	unlocked_ioctl : dummy_ioctl,
};

char devicename[20];

static struct cdev my_cdev;
static dev_t device_num;   // For device minor number
static struct class *cl;
struct semaphore sem;

// define custom single linkedlist

struct sll
{
	struct sll *next;
	int v;
};

// head of linkedlist
struct sll *head;
unsigned int size;

/*static void printSll(void)
{
	// print linkedlist, purpose of debugging
	int i;
	struct sll *cur;
	
	printk("Size : %d\n",size);
	
	cur = head;
	for(i=0;i<size;i++)
	{
		printk("%d node : %d\n",i,cur->v);
		cur = cur->next;
	}
}*/

// functions for linkedlist
static void allocLinkedListRandomly(int v)
{
	struct sll *cur;
	struct sll *temp;
	unsigned int pos;
	
	// acquire lock
	down(&sem);	
	// head is null, special case
	if(head == NULL)
	{
		head = (struct sll*)kzalloc(sizeof(struct sll),GFP_KERNEL);
		
		head->v = v;
	}
	else
	{
		cur = head;
		
		// get random number;
		get_random_bytes(&pos,sizeof(unsigned int));
		pos = pos % size;
		
		// move ahead for 'pos' times
		while(pos--)
		{
			cur = cur->next;
		}
		// if cur points tail, then just allocate
		if(cur->next == NULL)
		{
			cur->next = (struct sll*)kzalloc(sizeof(struct sll),GFP_KERNEL);
			cur->next->v = v;
		}
		else
		{
			// if cur is not tail, allocate and concatenate
			temp = cur->next;
			cur->next = (struct sll*)kzalloc(sizeof(struct sll),GFP_KERNEL);
			cur->next->v = v;
			cur->next->next = temp;
		}
	}
	size++;
	// release lock
	up(&sem);
}

static int getFirstMatchedSllNodeWithDeleteOperation(int v)
{
	int ret = -1,i,matched = 0;
	struct sll *cur;
	struct sll *temp;
	
	// acquire lock
	down(&sem);
	if(size > 0)
	{
		// special case, head node is matched
		if(head->v == v)
		{
			ret = v;
			matched = 1;
			
			// delete head
			temp = head;
			head = head->next;
			
			kfree(temp);			
			size--;
		}
		else
		{
			// search from head->next, it is necessary, because delete operation requires prev node.
			cur = head;
			
			for(i=0;i<size-1;i++)
			{
				if(cur->next->v == v)
				{
					ret = v;
					matched = 1; // matched
					break;
				}
				cur = cur->next;
			}
			if(matched == 1)
			{
				// delete matched node
				temp = cur->next;
				cur->next = cur->next->next; // concatenate prev and next node of target node.
				
				kfree(temp);
				size--;
			}
		}
	}
	// release lock
	up(&sem);
	return ret;
}

// release nodes
static void releaseSll(void)
{
	int i;
	struct sll *curptr, *nextptr;
	
	down(&sem);
	curptr = head;
	for(i=0;i<size;i++)
	{
		nextptr = curptr->next;
		kfree(curptr);
		curptr = nextptr;
	}
	size = 0;
	head = NULL;
	up(&sem);
}

static int __init dummy_init(void)
{

	printk("Dummy Driver : Module Init\n");
	strcpy(devicename, DUMMY_DEVICE_NAME);

	// Allocating device region 
	if (alloc_chrdev_region(&device_num, 0, 1, devicename)){
		return -1;
	}
	if ((cl = class_create(THIS_MODULE, "chardrv" )) == NULL) {
		unregister_chrdev_region(device_num, 1);
		return -1;
	}

	// Device Create == mknod /dev/DUMMY_DEVICE 
	if (device_create(cl, NULL, device_num, NULL, devicename) == NULL)	{
		class_destroy(cl);
		unregister_chrdev_region(device_num, 1);
		return -1;
	}
	// doit :P , init semaphore
	sema_init(&sem,1);	
	// doit :P, init sll variables
	head = NULL;
	size = 0;
	// Device Init 
	cdev_init(&my_cdev, &dummy_fops);
	if ( cdev_add(&my_cdev, device_num, 1) == -1 ){
		device_destroy(cl, device_num);
		class_destroy(cl);
		unregister_chrdev_region(device_num, 1);
	}
	
	return 0;
}

static void __exit dummy_exit(void)
{
	printk("Dummy Driver : Clean Up Module\n");
	cdev_del(&my_cdev);
	device_destroy(cl, device_num);
	class_destroy(cl);
	unregister_chrdev_region(MKDEV(DUMMY_MAJOR_NUMBER,0),128);
}

ssize_t dummy_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
	int ret, v;
	
	if(length > 1)
	{
		// request from user
		v = buffer[0];
		
		// search target value from linkedlist
		ret = getFirstMatchedSllNodeWithDeleteOperation(v);
		
		// Fail to find
		if(ret == -1)
		{
			return 0;
		}
		// succeed to find
		buffer[1] = ret;
		return 1;		
	}
	return 0;
}

ssize_t dummy_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
	int v;
	
	if(length > 0)
	{
		// request from user
		v = buffer[0];
		
		if(0<= v && v <=9)
		{
			// insert the value from the user in random position
			allocLinkedListRandomly(v);
			return 1;
		}
		else
		{
			return 0;
		}
	}
	return 0;
}

int dummy_open(struct inode *inode, struct file *file)
{
	printk("Dummy Driver : Open Call\n");
	return 0;
}

int dummy_release(struct inode *inode, struct file *file)
{
	printk("Dummy Driver : Release Call\n");
	//printSll();
	releaseSll();
	return 0;
}

long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long argument)
{
	printk("ioctl");
	return 0;
}


module_init(dummy_init);
module_exit(dummy_exit);

MODULE_DESCRIPTION("Dummy_LinkedList_Driver");
MODULE_LICENSE("GPL");

