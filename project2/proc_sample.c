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

#include <linux/task_io_accounting_ops.h>

#define FILE_NAME "proc_sample"
#define BUF_SIZE 512
#define MAX_PROCESS_COUNT 2048

char mybuf[BUF_SIZE];

struct mydata{
	int pid;
	char name[32];
	int virt;
	int rss;
	int diskread;
	int diskwrite;
	int totalio;
};

static int procmon_proc_show(struct seq_file *m, void *v)
{
	struct mydata* dataset;
	int i = 0,processCount = 0;
	seq_printf(m, "======= Contents ====== \n");
	seq_printf(m, "PID\t\tProcessName\tVIRT(KB)\tRSS Mem(KB)\tDiskRead(KB)\tDiskWrite(KB)\tTotal I/O(KB)\n"); 
	struct task_struct* p;
	
	dataset = (struct mydata*)kzalloc(sizeof(struct mydata)*MAX_PROCESS_COUNT,GFP_KERNEL);
	
	for_each_process(p)
	{
		dataset[processCount].pid = p->pid;
		
		sprintf(dataset[processCount].name,"%s",p->comm);
		
		if(p->active_mm == 0)
		{
			dataset[i].virt = dataset[i].rss = 0;
		}
		else
		{
			dataset[i].virt = p->active_mm->total_vm;
		
			dataset[i].rss = (p->active_mm->rss_stat.count[MM_FILEPAGES].counter)+(p->active_mm->rss_stat.count[MM_ANONPAGES].counter);
		}
		
		struct task_io_accounting target_io;
		task_io_accounting_init(&target_io);
		
		struct task_struct* p_thread_ptr;
		
		task_io_accounting_add(&target_io,&(p->ioac));
		
		task_io_accounting_add(&target_io,&(p->signal->ioac));
		
		for_each_thread(p,p_thread_ptr)
		{
			task_io_accounting_add(&target_io,&(p_thread_ptr->ioac));
		}
		dataset[processCount].diskread = target_io.read_bytes / 1024;
		dataset[processCount].diskwrite = target_io.write_bytes / 1024;
		
		
		dataset[processCount].totalio = dataset[processCount].diskread + dataset[processCount].diskwrite;
		processCount++;
	}
	
	for(i=0;i<processCount;i++)
	{
		seq_printf(m,"%d\t%16s\t%8d\t%8d\t%8d\t%8d\t%8d\n",dataset[i].pid,dataset[i].name,dataset[i].virt,dataset[i].rss,dataset[i].diskread,dataset[i].diskwrite,dataset[i].totalio);
	}
	kfree(dataset);
	return 0;
}



static int procmon_proc_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "proc called open\n");
	return single_open(file, procmon_proc_show, NULL);
}



static ssize_t procmon_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{

	memset(mybuf, 0, sizeof(mybuf));

	if (count > BUF_SIZE) {
		count = BUF_SIZE;
	}

	if (copy_from_user(mybuf, buf, count)) {
		return -EFAULT;
	}

	printk(KERN_INFO "proc write : %s\n", mybuf); 
	return (ssize_t)count;
}




struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = procmon_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = procmon_proc_write,
	.release = single_release,
};


static int __init init_procmon (void)
{
	
	struct proc_dir_entry *procmon_proc;


	procmon_proc = proc_create(FILE_NAME, 644, NULL, &fops);
	if (!procmon_proc) {
		printk(KERN_ERR "==Cannot create procmon proc entry \n");
		return -1;
	}
	printk(KERN_INFO "== init procmon\n");
	return 0;
}

static void __exit exit_procmon(void)
{
	remove_proc_entry(FILE_NAME, NULL);
	printk(KERN_INFO "== exit procmon\n");
}



module_init(init_procmon);
module_exit(exit_procmon);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dong-Jae Shin");
MODULE_DESCRIPTION("EE516 Project2 Process Monitoring Module");
