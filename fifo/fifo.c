
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

//misc device struct
static struct miscdevice my_device;

//number of open devices
static int open_count;

//buffer size to store the characters
static int buffer_size;

//string to store device name
char* device_name;

//number of characters in each string
static int string_char_count;

//read and write index in the buffer
static int read_index = 0, write_index = 0;

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

//initialize module, allocate memory, initialize semaphores ...*/
int init_module(){
	//initializing parameters for my_device
	my_device.name = device_name;
	my_device.minor = MISC_DYNAMIC_MINOR;
	my_device.fops = &my_device_fops;
	
	//register the device
	int register_return_value;
	if((register_return_value = misc_register(&my_device))){
		printk(KERN_ERR "Could not register the device\n");
		return register_return_value;
	}
	printk(KERN_INFO "Device Registered!\n");
	printk(KERN_INFO "Device Details\n--------------\n");
	printk(KERN_INFO "device name: %s\n", device_name);
	printk(KERN_INFO "buffer size: %d\n", buffer_size);
	printk(KERN_INFO "--------------\n");
	
	//allocating memory for the buffer
	int _allocated = 0;
	buffer = (char**)kmalloc(buffer_size*sizeof(char*), GFP_KERNEL);
	while(_allocated < buffer_size){
		buffer[_allocated] = (char*)kmalloc((string_char_count+1)*sizeof(char), GFP_KERNEL);
		buffer[string_char_count] = '\0';
		++_allocated;
	}

	//initialization of buffer attributes and mutators
	sema_init(&full, 0);
	sema_init(&empty, buffer_size);
	sema_init(&read_op_mutex, 1);
	sema_init(&write_op_mutex, 1);
	buffer_empty_slots = buffer_size;
	open_count = 0;
	return 0;
}



