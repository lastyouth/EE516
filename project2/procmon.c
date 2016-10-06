#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include <linux/ctype.h>
#include <linux/task_io_accounting_ops.h>

#define FILE_NAME "procmon"
#define SELECTION_FILE_NAME "procmon_sorting"
#define BUF_SIZE 512
#define MAX_PROCESS_COUNT 2048

char sortingBuf[BUF_SIZE]; // for /proc/procmon_sorting.
char normalBuf[BUF_SIZE]; // for /proc/procmon, prevent it from blocking when user uses echo command.

// output should be sorted by specific criterion.
struct mydata{
	int pid;
	char name[32];
	long virt;
	long rss;
	long diskread;
	long diskwrite;
	long totalio;
};

// enum type for sorting options.
enum{
	 PID,
	 VIRT,
	 RSS,
	 IO};

// variable specifies current sorting type
static int currentValue;

void bubblesort(struct mydata* data, int len)
{
	/*
	 * Using bubble sort algorithm to perform sorting simply
	 * Sorting criterion is used in switch logic to determine the comparison logic*/
	int i,j;
	for(i = 0;i<len -1;i++)
	{
		for(j=0;j<len-i-1;j++)
		{
			switch(currentValue)
			{
				case PID:
					if(data[j].pid > data[j+1].pid)
					{
						struct mydata temp;
						temp = data[j];
						data[j] = data[j+1];
						data[j+1] = temp;
					}
				break;
				case VIRT:
					if(data[j].virt < data[j+1].virt)
					{
						struct mydata temp;
						temp = data[j];
						data[j] = data[j+1];
						data[j+1] = temp;
					}
				break;
				case RSS:
					if(data[j].rss < data[j+1].rss)
					{
						struct mydata temp;
						temp = data[j];
						data[j] = data[j+1];
						data[j+1] = temp;
					}
				break;
				case IO:
					if(data[j].totalio < data[j+1].totalio)
					{
						struct mydata temp;
						temp = data[j];
						data[j] = data[j+1];
						data[j+1] = temp;
					}
				break;
			}
		}
	}
}

static int procmon_proc_show(struct seq_file *m, void *v)
{
	/*
	 * this function is activated when user performs shell script command 'cat /proc/procmon' after loading module
	 * */
	struct mydata* dataset; // for saving intermediate result of processes.
	
	int i = 0,processCount = 0; // i for 'for loop', processCount is for number of processes.
	
	struct task_struct* p; // pointer for 'for_each_process' macro.
	
	struct mm_struct *amm; // To refer mm_struct in task_struct, prevent code to be long.
	
	long filepages, anonpages; // Result from rss_stat[index]
	
	struct task_io_accounting target_io; // To gather IO stat from process 
	
	struct task_struct* p_thread_ptr; // for 'for_each_thread', it points each thread of process
	
	seq_printf(m, "======= Contents ====== \n");
	seq_printf(m, "PID\t\tProcessName\tVIRT(KB)\tRSS Mem(KB)\tDiskRead(KB)\tDiskWrite(KB)\tTotal I/O(KB)\n"); 

	// use kzalloc for initalizing zero in allocated memory.
	dataset = (struct mydata*)kzalloc(sizeof(struct mydata)*MAX_PROCESS_COUNT,GFP_KERNEL);
	
	for_each_process(p)
	{
		/* this routine is executed per each process.
		 * */
		dataset[processCount].pid = p->pid; // copy pid
			
		sprintf(dataset[processCount].name,"%s",p->comm); // copy process name
		
		dataset[processCount].virt = dataset[processCount].rss = 0; 
		
		if(p->active_mm) 
		{
			/*
			 * this routine is executed, if mm_struct pointer in task_struct exists*/
			amm = p->active_mm;
			
			if(amm->total_vm)
			{
				dataset[processCount].virt = amm->total_vm*PAGE_SIZE/1024; // unit : KB
			}
					
			filepages = atomic_long_read(&(amm->rss_stat.count[MM_FILEPAGES]));
			anonpages = atomic_long_read(&(amm->rss_stat.count[MM_ANONPAGES]));
			
			if(filepages < 0) // ignore negative value of MM_FILEPAGES
			{
				//printk("negative filepages size : %ld\n",filepages);
			}
			else
			{
				dataset[processCount].rss += filepages;
			}
			if(anonpages < 0) // ignore negative value of MM_ANONPAGES
			{
				//printk("negative anonpages size : %ld\n",anonpages);
			}
			else
			{
				dataset[processCount].rss += anonpages;
			}
			
			dataset[processCount].rss *= PAGE_SIZE / 1024;  // unit : KB
		}

		task_io_accounting_init(&target_io); // initalize task_io_accounting structure for add operation
				
		task_io_accounting_add(&target_io,&(p->signal->ioac)); // for gathering IO stats
		for_each_thread(p,p_thread_ptr)
		{
			task_io_accounting_add(&target_io,&(p_thread_ptr->ioac)); // for gathering IO stats
		}
		dataset[processCount].diskread = target_io.read_bytes / 1024; // unit : KB
		dataset[processCount].diskwrite = target_io.write_bytes / 1024; // unit : KB
		
		
		dataset[processCount].totalio = dataset[processCount].diskread + dataset[processCount].diskwrite;
		processCount++;
	}
	// sort by currentValue
	bubblesort(dataset,processCount);
	
	// print routine
	for(i=0;i<processCount;i++)
	{
		seq_printf(m,"%d\t%16s\t%8ld\t%8ld\t%8ld\t%8ld\t%8ld\n",dataset[i].pid,dataset[i].name,dataset[i].virt,dataset[i].rss,dataset[i].diskread,dataset[i].diskwrite,dataset[i].totalio);
	}
	// free allocated memory
	kfree(dataset);
	return 0;
}



static int procmon_proc_open(struct inode *inode, struct file *file)
{
	// trigger for procmon_proc_show
	printk(KERN_INFO "proc called open\n");
	return single_open(file, procmon_proc_show, NULL);
}


static ssize_t procmon_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	/*
	 * dummy routine for preventing block by using echo command to /proc/procmon
	 * */
	memset(normalBuf, 0, sizeof(normalBuf));

	if (count > BUF_SIZE) {
		count = BUF_SIZE;
	}

	if (copy_from_user(normalBuf, buf, count)) {
		return -EFAULT;
	}

	printk(KERN_INFO "proc write : %s\n", normalBuf); 
	return (ssize_t)count;
}



static int procmon_proc_selection_show(struct seq_file *m, void *v)
{
	/*
	 * triggered by procmon_selection_open
	 * */
	 
	// print current sorting criterion by currentValue variable
	switch(currentValue)
	{
		case PID:
			seq_printf(m,"[PID] VIRT RSS IO\n");
			break;
		case VIRT:
			seq_printf(m,"PID [VIRT] RSS IO\n");
			break;
		case RSS:
			seq_printf(m,"PID VIRT [RSS] IO\n");
			break;
		case IO:
			seq_printf(m,"PID VIRT RSS [IO]\n");
			break;
		default:
			seq_printf(m,"What a Terrible Failure : WTF\n");
	}
	
	return 0;
}


static int procmon_selection_open(struct inode *inode, struct file *file)
{
	// trigger for procmon_proc_selection_show function.
	printk(KERN_INFO "procmon_selection_open called open\n");
	return single_open(file, procmon_proc_selection_show, NULL);
}

static ssize_t procmon_proc_selection_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	/*
	 * function for selecting sorting criterion*/
	int len, i;
	memset(sortingBuf, 0, sizeof(sortingBuf));

	if (count > BUF_SIZE) {
		count = BUF_SIZE;
	}

	if (copy_from_user(sortingBuf, buf, count)) {
		return -EFAULT;
	}
	len = strlen(sortingBuf);
	for(i=len-1;i>=0;i--)
	{
		if(isspace(sortingBuf[i]))
		{
			sortingBuf[i] = '\0';
		}
		else
		{
			break;
		}
	}
	if(strcmp(sortingBuf,"pid")==0)
    {
		currentValue = PID;
    }else if(strcmp(sortingBuf,"virt")==0)
    {
		currentValue = VIRT;
    }else if(strcmp(sortingBuf,"rss")==0)
    {
		currentValue = RSS;
    }else if(strcmp(sortingBuf,"io")==0)
    {
		currentValue = IO;
    }else
    {
		printk(KERN_ERR "UNKNOWN OPERATION : %s\n", sortingBuf);
    }
	printk(KERN_INFO "proc write : %s, currentValue : %d\n", sortingBuf,currentValue); 
	return (ssize_t)count;
}

// file_operations for /proc/procmon
struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = procmon_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = procmon_proc_write,
	.release = single_release,
};

// file_operations for /proc/procmon_sorting
struct file_operations f_option_ops = {
	.owner = THIS_MODULE,
	.open = procmon_selection_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = procmon_proc_selection_write,
	.release = single_release,
};

static int __init init_procmon (void)
{
	// triggered by insmod procmon.ko
	struct proc_dir_entry *procmon_proc;
	struct proc_dir_entry *procmon_selection;

	currentValue = PID;

	procmon_proc = proc_create(FILE_NAME, 644, NULL, &fops);
	if (!procmon_proc) {
		printk(KERN_ERR "==Cannot create procmon proc entry \n");
		return -1;
	}
	
	
	procmon_selection = proc_create(SELECTION_FILE_NAME,644,NULL,&f_option_ops);
	if (!procmon_selection) {
		printk(KERN_ERR "==Cannot create procmon proc entry \n");
		return -1;
	}
	printk(KERN_INFO "== init procmon\n");
	return 0;
}

static void __exit exit_procmon(void)
{
	// triggered by rmmod procmon
	remove_proc_entry(FILE_NAME, NULL);
	remove_proc_entry(SELECTION_FILE_NAME, NULL);
	printk(KERN_INFO "== exit procmon\n");
}



module_init(init_procmon);
module_exit(exit_procmon);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Skeleton : Dong-Jae Shin, Modified : BoHun Seo");
MODULE_DESCRIPTION("EE516 Project2 Process Monitoring Module");
