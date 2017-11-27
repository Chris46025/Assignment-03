
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/sched.h>


//buffer slots to read and write
static int buffer_empty_slots;

//sempaphore for buffer
static struct semaphore full;
static struct semaphore empty;

//semaphore for read/write operations
static struct semaphore read_op_mutex;
static struct semaphore write_op_mutex;

//buffer to store strings
char** buffer;

//declaring file operation functions
static int fifo_open(struct inode*, struct file*);
static ssize_t fifo_read(struct file*, char*, size_t, loff_t*);
static ssize_t fifo_write(struct file*, const char*, size_t, loff_t*);
static int fifo_release(struct inode*, struct file*);

static struct file_operations my_device_fops = {
	.open = &fifo_open,
	.read = &fifo_read,
	.write = &fifo_write,
	.release = &fifo_release
};
