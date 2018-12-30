#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kfifo.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/io.h>

extern void spkr_init(void);
extern void spkr_exit(void);
extern void spkr_set_frequency(unsigned int frequency);
extern void spkr_on(void);
extern void spkr_off(void);

dev_t dev;
struct cdev char_device;
struct class* module;

static int open_mod(struct inode *inode, struct file *file);
static int release_mod(struct inode *inode, struct file *flip);
static ssize_t write(struct file *flip, const char __user *buf, size_t count, loff_t *f_pos);

static unsigned int minor = 0;
static unsigned int frequency = 440;
static int status;
static struct file_operations fops = {
      .owner =    THIS_MODULE,
      .open =     open_mod,
      .release =  release_mod,
      .write =    write
};

#define CLASS_NAME "speaker"
#define DEVICE_NAME "intspkr"
#define MODULE_NAME "spkr"

MODULE_AUTHOR("Abraham Carrera and Jorge Forcada");
MODULE_DESCRIPTION("Speaker driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("Dual BSD/GPL");

module_param(minor, int, S_IRUGO);
module_param(frequency, int, S_IRUGO);

static int __init intspkr_init(void) {
  printk(KERN_INFO "Executing: intspkr_init\n\n");

  status=alloc_chrdev_region(&dev, minor, 1, MODULE_NAME);

  if(status<0){
    printk(KERN_ALERT "Error allocating major or minor\n");

    printk(KERN_INFO "\nError: intspkr_init\n\n");
    return -1;
  }

   printk(KERN_ALERT "MAJOR ASIGNED: %d\n", (int)MAJOR(dev));
   printk(KERN_ALERT "MINOR ASIGNED: %d\n", (int)MINOR(dev));

  cdev_init(&char_device,  &fops);
  cdev_add(&char_device, dev, 1);
  module = class_create(THIS_MODULE, CLASS_NAME);
  device_create(module, NULL, dev, NULL, DEVICE_NAME);
  printk(KERN_INFO "\nSuccess: intspkr_init\n\n");

  spkr_init();
  spkr_set_frequency(frequency);
  spkr_on();

  return 0;
}

static void __exit  intspkr_exit(void) {
  printk(KERN_INFO "Executing: intspkr_exit\n\n");

   spkr_off();
   spkr_exit();

  cdev_del(&char_device);
  unregister_chrdev_region(dev, 1);

  device_destroy(module, dev);
  class_destroy(module);

  printk(KERN_INFO "\nSuccess: intspkr_exit\n\n");
}

static int open_mod(struct inode *inode, struct file *filp) {
    printk(KERN_ALERT "Open call\n");
    return 0;
}

static int release_mod(struct inode *inode, struct file *filp) {
    printk(KERN_ALERT "Release call\n");
    return 0;
}

static ssize_t write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    printk(KERN_ALERT "Write call\n");
    return 0;
}

module_init(intspkr_init);
module_exit(intspkr_exit);
