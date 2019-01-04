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
#include <linux/errno.h>

extern void spkr_io_init(void);
extern void spkr_io_exit(void);
extern void spkr_set_frequency(unsigned int frequency);
extern void spkr_on(void);
extern void spkr_off(void);

dev_t dev;
struct cdev char_device;
struct class* module;

struct info_mydev {
  struct cdev mydev_cdev;
};

static int open_mod(struct inode *inode, struct file *filp);
static int release_mod(struct inode *inode, struct file *flip);
static ssize_t write(struct file *flip, const char __user *buf, size_t count, loff_t *f_pos);
static int mydev_open(struct inode *inode, struct file *filp);
static void mydev_release(struct inode *inode, struct file *filp);

struct info_mydev *info_dev;
static int is_busy = 0;
static int open_result;
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

static int __init spkr_init(void) {
  printk(KERN_INFO "Executing: spkr_init\n\n");

  status=alloc_chrdev_region(&dev, minor, 1, MODULE_NAME);

  if(status<0){
    printk(KERN_ALERT "Error allocating major or minor\n");

    printk(KERN_INFO "\nError: spkr_init\n\n");
    return -1;
  }

  printk(KERN_INFO "MAJOR ASIGNED: %d\n", (int)MAJOR(dev));
  printk(KERN_INFO "MINOR ASIGNED: %d\n", (int)MINOR(dev));

  cdev_init(&char_device,  &fops);
  cdev_add(&char_device, dev, 1);
  module = class_create(THIS_MODULE, CLASS_NAME);
  device_create(module, NULL, dev, NULL, DEVICE_NAME);

  printk(KERN_INFO "\nSuccess: spkr_init\n\n");

  spkr_io_init();
  spkr_set_frequency(frequency);
  spkr_on();

  return 0;
}

static void __exit spkr_exit(void) {
  printk(KERN_INFO "Executing: spkr_exit\n\n");

  spkr_off();
  spkr_io_exit();

  cdev_del(&char_device);
  unregister_chrdev_region(dev, 1);

  device_destroy(module, dev);
  class_destroy(module);

  printk(KERN_INFO "\nSuccess: spkr_exit\n\n");
}

static int open_mod(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "Executing: open_mod\n\n");

  open_result = mydev_open(inode, filp);

  if (open_result < 0) {
    printk(KERN_INFO "\nError: open_mod %d\n\n", open_result);
  } else {
    printk(KERN_INFO "\nSuccess: open_mod\n\n");
  }
  return open_result;
}

static int release_mod(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "Executing: release_mod\n\n");
  mydev_release(inode, filp);
  printk(KERN_INFO "\nSuccess: release_mod\n\n");
  return 0;
}

static ssize_t write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
  printk(KERN_INFO "Executing: write\n\n");
  // To do...
  printk(KERN_INFO "\nSuccess: write\n\n");
  return 0;
}

static int mydev_open(struct inode *inode, struct file *filp) {
  if (((filp->f_flags & O_ACCMODE) == O_WRONLY) && is_busy){
    return -EBUSY;
  }

  info_dev = container_of(inode->i_cdev, struct info_mydev, mydev_cdev);
  filp->private_data = info_dev;

  if ((filp->f_flags & O_ACCMODE) == O_WRONLY){
    is_busy = 1;
  }

  return 0;
}

static void mydev_release(struct inode *inode, struct file *filp) {
  if ((filp->f_flags & O_ACCMODE) == O_WRONLY){
    is_busy = 0;
  }
}

module_init(spkr_init);
module_exit(spkr_exit);
