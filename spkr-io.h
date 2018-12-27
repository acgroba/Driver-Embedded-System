#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

dev_t dev;
struct cdev char_device;
struct class* module;


static int open_mod(struct inode *inode, struct file *file);
static int release_mod(struct inode *inode, struct file *flip);
static ssize_t write(struct file *flip, const char __user *buf,
		     size_t count, loff_t *f_pos);
