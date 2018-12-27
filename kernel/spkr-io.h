#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

int intspkr_init(void);
void intspkr_exit(void);
void intspkr_set_frequency(unsigned int frequency);
void intspkr_on(void);
void intspkr_off(void);

int open_mod(struct inode *inode, struct file *file);
int release_mod(struct inode *inode, struct file *file);
ssize_t write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
