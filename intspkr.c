#include "intspkr.h"


MODULE_AUTHOR("Abraham Carrera and Jorge Forcada");
MODULE_DESCRIPTION("Speaker driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("Dual BSD/GPL");

static unsigned int minor = 0;
module_param(minor, int, S_IRUGO);


static int __init intspkr_init(void)
{
  int status;

  status=alloc_chrdev_region(&dev, minor,
                        1, "intspkr");
  
  if(status<0){
    printk(KERN_ALERT "Error allocating major or minor\n");

    return -1;
  }
  
  

   printk(KERN_ALERT "MAJOR ASIGNED: %d\n", (int)MAJOR(dev));
   printk(KERN_ALERT "MINOR ASIGNED: %d\n", (int)MINOR(dev));
  static struct file_operations fops = {
        .owner =    THIS_MODULE,
        .open =     open_mod,
        .release =  release_mod,
        .write =    write
};

 cdev_init(&char_device,  &fops);
 cdev_add(&char_device, dev, 1);
 module = class_create(THIS_MODULE, "speaker");
 device_create(module, NULL, dev, NULL, "intspkr");
  return 0;
}

static void __exit  intspkr_exit(void)
{
  cdev_del(&char_device);
  unregister_chrdev_region(dev, 1);

  device_destroy(module, dev);
  class_destroy(module);
  
  
  printk(KERN_ALERT "Exit module  \n");

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