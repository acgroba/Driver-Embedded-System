#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/delay.h>


#define PERIPHERICAL_BASE_DIR     0x3F000000
#define PERIPHERICAL_SPACE_SIZE   0x01000000
#define CLOCKS_DISPLACEMENT		  0x101000
#define CM_PWMCLKCTL              0xA0
#define CM_PWMCLKDIV              0xA4
#define PASSWORD                 (0x5A << 24)
#define DIVIDER                   16
#define GPIO_DISPLACEMENT		  0x200000
#define GPFSEL1_DISPLACEMENT      4
#define GPFSEL1_VALUE			  0X02
#define GPFSEL1_MASK			  0X07
#define PIN 					  18
#define PWM_DISPLACEMENT 		  0x20C000
#define FREQ                      1200000
#define PWM_RANGE_DISPLACEMENT 	  0x10
#define PWM_DATA_DISPLACEMENT 	  0x14
#define PWM_CONTROL_DISPLACEMENT  0
#define PWM_MS_MODE               0x0080
#define PWM_ENABLE                0x0001
#define MAP_FAILED                NULL


static uint32_t *virtual_dir_peripherals;
static volatile uint32_t *dir_clk;
static volatile uint32_t *dir_gpio;
static volatile uint32_t *dir_pwm;
static volatile uint32_t* gpfsel1_dir;

static  uint8_t  desp;
static  uint32_t ctrl;
static  uint32_t range;
static  uint32_t data;
static  uint32_t val;
static  uint32_t value;
static  uint32_t mask;


static void init_clk(void);
static void init_gpio(void);
static void reg_write_sync(volatile uint32_t* address, uint32_t value);
static void reg_write_no_sync(volatile uint32_t* address, uint32_t value);
static uint32_t reg_read_sync(volatile uint32_t* address);

void spkr_io_init(void) {
	printk(KERN_INFO "spkr init\n");
    virtual_dir_peripherals= (uint32_t*) ioremap(PERIPHERICAL_BASE_DIR, PERIPHERICAL_SPACE_SIZE);

	init_clk();
	init_gpio();

}

void spkr_set_frequency(unsigned int frequency) {
	printk(KERN_INFO "spkr set frequency: %d\n", frequency);


    dir_pwm=(uint32_t *)virtual_dir_peripherals+PWM_DISPLACEMENT/4;

    if (dir_pwm == MAP_FAILED){
      printk(KERN_INFO "ERROR 1\n");
      return;}

    range=FREQ/frequency;
    reg_write_no_sync( (uint32_t *)dir_pwm+PWM_RANGE_DISPLACEMENT/4,range);


    data = range/2;
    reg_write_no_sync( (uint32_t *) dir_pwm+PWM_DATA_DISPLACEMENT/4,data);

}


void spkr_on(void) {
	printk(KERN_INFO "spkr ON\n");

    if (dir_pwm == MAP_FAILED){
    	   printk(KERN_INFO "ERROR 2\n");

      return;
    }
     ctrl = reg_read_sync((uint32_t *)dir_pwm + PWM_CONTROL_DISPLACEMENT);
    ctrl |= PWM_MS_MODE;
    ctrl |= PWM_ENABLE;
    reg_write_no_sync((uint32_t *) dir_pwm + PWM_CONTROL_DISPLACEMENT, ctrl);


}
void spkr_off(void) {
	printk(KERN_INFO "spkr OFF\n");

        if (dir_pwm == MAP_FAILED){
        	printk(KERN_INFO "ERROR 3\n");
        }


    ctrl = reg_read_sync((uint32_t *) dir_pwm + PWM_CONTROL_DISPLACEMENT);
    ctrl &= ~PWM_MS_MODE;
    ctrl &= ~PWM_ENABLE;
    reg_write_no_sync((uint32_t *) dir_pwm + PWM_CONTROL_DISPLACEMENT, ctrl);

}

void spkr_io_exit(void) {
	printk(KERN_INFO "spkr exit\n");

	 iounmap(virtual_dir_peripherals);

    virtual_dir_peripherals = MAP_FAILED;
    dir_clk = MAP_FAILED;
    dir_gpio  = MAP_FAILED;
    dir_pwm  = MAP_FAILED;


}


void init_clk(void){

    dir_clk=virtual_dir_peripherals+CLOCKS_DISPLACEMENT/4;

    if (dir_clk == MAP_FAILED){
    	printk(KERN_INFO "ERROR 4\n");
      return;
    }

    reg_write_sync((uint32_t *)dir_clk + CM_PWMCLKDIV/4, PASSWORD | (DIVIDER << 12));
    mdelay(100);
    reg_write_sync((uint32_t *) dir_clk + CM_PWMCLKCTL/4, PASSWORD | 0x11);
}

void init_gpio(void){

	dir_gpio=virtual_dir_peripherals+GPIO_DISPLACEMENT/4;

	 gpfsel1_dir = (uint32_t *)dir_gpio + (PIN/10);
     desp = (PIN % 10) * 3;
     mask = GPFSEL1_MASK << desp;
     val = GPFSEL1_VALUE << desp;

     value = reg_read_sync(gpfsel1_dir);
     value = (value & ~mask) | (val & mask);
     reg_write_sync(gpfsel1_dir, value);

}

void reg_write_sync(volatile uint32_t* address, uint32_t value)
{
  __sync_synchronize();
  *address = value;
  __sync_synchronize();
}

void reg_write_no_sync(volatile uint32_t* address, uint32_t value)
{

  *address = value;

}

uint32_t reg_read_sync(volatile uint32_t* address)
{
    uint32_t toret;
    __sync_synchronize();
    toret = *address;
    __sync_synchronize();
    return toret;
}




