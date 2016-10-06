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

#define FILE_NAME "proc_sample"
#define BUF_SIZE 512

char mybuf[BUF_SIZE];


static int procmon_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "======= Contents ====== \n");
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
