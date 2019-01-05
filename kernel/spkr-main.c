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
#include <linux/spinlock.h>

extern void spkr_io_init(void);
extern void spkr_io_exit(void);
extern void spkr_set_frequency(unsigned int frequency);
extern void spkr_on(void);
extern void spkr_off(void);

#define CLASS_NAME "speaker"
#define DEVICE_NAME "intspkr"
#define MODULE_NAME "spkr"
#define SOUND_SIZE 4

dev_t dev;
struct cdev char_device;
struct class* module;

struct info_mydev {
  struct cdev mydev_cdev;
};

struct wait_queue {
  wait_queue_head_t list;
};

static int open_mod(struct inode *inode, struct file *filp);
static int release_mod(struct inode *inode, struct file *flip);
static ssize_t write(struct file *flip, const char __user *buf, size_t count, loff_t *f_pos);
static int mydev_open(struct inode *inode, struct file *filp);
static void mydev_release(struct inode *inode, struct file *filp);
static int power_of_2(int n);
static void play_sound(void);
static void timer_function(unsigned long data);
static unsigned int lower(unsigned int threshold, unsigned int bytes_left);

int is_playing;
static DEFINE_MUTEX(write_lock);
size_t writing_size;
size_t remaining_bytes = 0;
int sz;
int err;
struct timer_list spkr_timer;
unsigned char sound[SOUND_SIZE];
int sound_freq, sound_time;
int bytes_copied, total_bytes_copied;
struct wait_queue spkr_write_fifo;
struct kfifo spkr_fifo;
int buffer_size = PAGE_SIZE;
int buffer_threshold = PAGE_SIZE;
spinlock_t file_mod_lock, write_to_device_lock;
struct info_mydev *info_dev;
static int is_busy = 0;
static int open_result;
static unsigned int minor = 0;
static struct file_operations fops = {
      .owner =    THIS_MODULE,
      .open =     open_mod,
      .release =  release_mod,
      .write =    write
};

module_param(buffer_threshold, int, S_IRUGO);
module_param(buffer_size, int, S_IRUGO);
module_param(minor, int, S_IRUGO);

MODULE_PARM_DESC(buffer_threshold, "buffer_threshold");
MODULE_PARM_DESC(buffer_size, "buffer_size");
MODULE_PARM_DESC(minor, "minor");
MODULE_AUTHOR("Abraham Carrera and Jorge Forcada");
MODULE_DESCRIPTION("Speaker driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("Dual BSD/GPL");

static int __init spkr_init(void) {
  printk(KERN_INFO "Executing: spkr_init\n\n");

  buffer_size = power_of_2(buffer_size);
  printk(KERN_INFO "Buffer Size: %d\n", buffer_size);

  if (buffer_threshold > buffer_size) {
    buffer_threshold = buffer_size;
  }

  if((err = alloc_chrdev_region(&dev, minor, 1, MODULE_NAME)) < 0){
    printk(KERN_ALERT "Error allocating major or minor: %d\n", err);

    printk(KERN_INFO "\nError: spkr_init\n");
    return -1;
  }

  printk(KERN_INFO "Major assigned: %d\n", (int)MAJOR(dev));
  printk(KERN_INFO "Minor assigned: %d\n", (int)MINOR(dev));

  cdev_init(&char_device,  &fops);
  cdev_add(&char_device, dev, 1);
  module = class_create(THIS_MODULE, CLASS_NAME);
  device_create(module, NULL, dev, NULL, DEVICE_NAME);

  if (kfifo_alloc(&spkr_fifo, buffer_size, GFP_USER)) {
    printk(KERN_ALERT "Error when kfifo_alloc: %d\n", err);
    printk(KERN_INFO "\nError: spkr_init\n");
    return -ENOMEM;
  }

  init_timer(&spkr_timer);
  spkr_timer.function = timer_function;
  spkr_timer.data = 0;

  init_waitqueue_head(&spkr_write_fifo.list);

  spkr_io_init();

  is_playing = 0;

  printk(KERN_INFO "\nSuccess: spkr_init\n");

  return 0;
}

static void __exit spkr_exit(void) {
  printk(KERN_INFO "Executing: spkr_exit\n\n");

  cdev_del(&char_device);
  unregister_chrdev_region(dev, 1);

  device_destroy(module, dev);
  class_destroy(module);

  kfifo_free(&spkr_fifo);
  del_timer_sync(&spkr_timer);

  spkr_off();
  spkr_io_exit();

  printk(KERN_INFO "\nSuccess: spkr_exit\n");
}

static int open_mod(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "Executing: open_mod\n");

  spin_lock_bh(&file_mod_lock);

  open_result = mydev_open(inode, filp);

  if (open_result < 0) {
    printk(KERN_INFO "Error: open_mod\n");
  } else {
    printk(KERN_INFO "Success: open_mod\n");
  }

  spin_unlock_bh(&file_mod_lock);

  return open_result;
}

static int release_mod(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "Executing: release_mod\n");

  spin_lock_bh(&file_mod_lock);

  mydev_release(inode, filp);

  spin_unlock_bh(&file_mod_lock);

  printk(KERN_INFO "Success: release_mod\n");
  return 0;
}

static ssize_t write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
  printk(KERN_INFO "Executing: write count: %zu bytes\n\n", count);

  bytes_copied = 0;
  total_bytes_copied = 0;

  if (mutex_lock_interruptible(&write_lock)) {
    return -ERESTARTSYS;
  }

  while (total_bytes_copied < count) {

    spkr_timer.data = count - total_bytes_copied;

    if(wait_event_interruptible(spkr_write_fifo.list, kfifo_avail(&spkr_fifo) > 0) != 0){
      return -ERESTARTSYS;
    }

    printk(KERN_INFO "Writer awaken");

    if (kfifo_from_user(&spkr_fifo, buf+total_bytes_copied, count, &bytes_copied) == -EFAULT) {
      return -EFAULT;
    }
    total_bytes_copied += bytes_copied;

    printk(KERN_INFO "Copied %d bytes\n", bytes_copied);

    if (!is_playing && kfifo_len(&spkr_fifo) >= SOUND_SIZE) {
      play_sound();
    }
  }

  mutex_unlock(&write_lock);

  printk(KERN_INFO "\nSuccess: write\n");
  return total_bytes_copied;
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

// It gets the base 2 logarithm, then add 1 to that.
static int power_of_2(int n) {
  n += (n == 0);
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}

static void play_sound(void) {
  printk(KERN_INFO "Executing: play_sound\n");

  is_playing = 1;

  spin_lock_bh(&write_to_device_lock);

  sz = kfifo_out(&spkr_fifo, sound, SOUND_SIZE);
  if (sz == 0) {
    printk(KERN_ALERT "Error when when kfifo_out: %d\n", err);
    printk(KERN_INFO "Error: play_sound\n");
    spin_unlock_bh(&write_to_device_lock);
    return;
  }
  spin_unlock_bh(&write_to_device_lock);

  sound_freq = (sound[0] | sound[1] << 8);
  sound_time = (sound[2] | sound[3] << 8);

  if(sound_freq == 0){
    spkr_off();
  } else {
    spkr_set_frequency(sound_freq);
    spkr_on();
  }

  spkr_timer.expires = jiffies + msecs_to_jiffies(sound_time);
  add_timer(&spkr_timer);

  printk(KERN_INFO "play_sound kfifo_len: %d\n", kfifo_len(&spkr_fifo));

  printk(KERN_INFO "Success: play_sound\n");
}

static void timer_function(unsigned long bytes_left) {

  if(kfifo_len(&spkr_fifo) >= SOUND_SIZE)
    play_sound();
  else{
    spkr_off();
    is_playing = 0;
  }

  if(kfifo_is_empty(&spkr_fifo) || kfifo_avail(&spkr_fifo) >= lower(buffer_threshold, bytes_left)){
    printk(KERN_INFO "Waking up writer");
    wake_up_interruptible(&spkr_write_fifo.list);
  }
}

static unsigned int lower(unsigned int threshold, unsigned int bytes_left) {
  if(threshold < bytes_left)
    return threshold;
  return bytes_left;
}

module_init(spkr_init);
module_exit(spkr_exit);
