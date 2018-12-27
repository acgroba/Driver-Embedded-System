#include "spkr-io.h"


MODULE_AUTHOR("Abraham Carrera and Jorge Forcada");
MODULE_DESCRIPTION("Speaker driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("Dual BSD/GPL");

dev_t dev;
struct cdev char_device;
struct class* module;

static unsigned int minor = 0;
static int status;
static struct file_operations fops = {
      .owner =    THIS_MODULE,
      .open =     open_mod,
      .release =  release_mod,
      .write =    write
};

module_param(minor, int, S_IRUGO);

int intspkr_init(void) {
  printk(KERN_INFO "Executing: intspkr_init\n\n");

  status=alloc_chrdev_region(&dev, minor, 1, "intspkr");

  if(status<0){
    printk(KERN_ALERT "Error allocating major or minor\n");

    printk(KERN_INFO "\nError: intspkr_init\n\n");
    return -1;
  }

  printk(KERN_ALERT "MAJOR ASIGNED: %d\n", (int)MAJOR(dev));
  printk(KERN_ALERT "MINOR ASIGNED: %d\n", (int)MINOR(dev));

  cdev_init(&char_device,  &fops);
  cdev_add(&char_device, dev, 1);
  module = class_create(THIS_MODULE, "speaker");
  device_create(module, NULL, dev, NULL, "intspkr");
  printk(KERN_INFO "\nSuccess: intspkr_init\n\n");
  return 0;
}

void intspkr_exit(void) {
  printk(KERN_INFO "Executing: intspkr_exit\n\n");

  cdev_del(&char_device);
  unregister_chrdev_region(dev, 1);

  device_destroy(module, dev);
  class_destroy(module);

  printk(KERN_INFO "\nSuccess: intspkr_exit\n\n");
}

void intspkr_set_frequency(unsigned int frequency) {
  printk(KERN_INFO "intspkr set frequency: %d\n", frequency);
}

void intspkr_on(void) {
  printk(KERN_INFO "intspkr ON\n");
}
void intspkr_off(void) {
  printk(KERN_INFO "intspkr OFF\n");
}

int open_mod(struct inode *inode, struct file *file) {
  printk(KERN_ALERT "Open call\n");
  return 0;
}

int release_mod(struct inode *inode, struct file *file) {
  printk(KERN_ALERT "Release call\n");
  return 0;
}

ssize_t write (struct file *file, const char __user *buf, size_t count, loff_t *f_pos) {
  printk(KERN_ALERT "Write call\n");
  return 0;
}

module_init(intspkr_init);
module_exit(intspkr_exit);