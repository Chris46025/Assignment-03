
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
static struct miscdevice fifo;

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

static struct file_operations fifo_fops = {
	.open = &fifo_open,
	.read = &fifo_read,
	.write = &fifo_write,
	.release = &fifo_release
};

//initialize module, allocate memory, initialize semaphores ...*/
int init_module(){
	//initializing parameters for my_device
	fifo.name = device_name;
	fifo.minor = MISC_DYNAMIC_MINOR;
	fifo.fops = &fifo_fops;
	
	//register the device
	int register_return_value;
	if((register_return_value = misc_register(&fifo))){
		printk(KERN_ERR "Could not register the device\n");
		return register_return_value;
	}
	printk(KERN_INFO "Device Registered!\n");
	
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

static int fifo_open(struct inode* _inode, struct file* _file){
	++open_count;
	return 0;
}

//Read file operation function, is called from the consumer
static ssize_t fifo_read(struct file* _file, char* user_buffer, size_t number_of_chars_to_be_read, loff_t* offset){
	int user_buffer_index = 0;
	
	down_interruptible(&read_op_mutex);
	down_interruptible(&full);
	
	read_index %= buffer_size;
	
	for(user_buffer_index = 0; user_buffer_index < number_of_chars_to_be_read; user_buffer_index++){
		if(buffer_empty_slots >= buffer_size){
			break;
		}
		copy_to_user(&user_buffer[user_buffer_index], &buffer[read_index][user_buffer_index], 1);
	}
	++read_index;
	++buffer_empty_slots;
	
	up(&empty);
	up(&read_op_mutex);
	
	return user_buffer_index;
}

//Write file operation function, is called from the producer
static ssize_t fifo_write(struct file* _file, const char* user_buffer, size_t number_of_chars_to_write, loff_t* offset){
	int user_buffer_index = 0;
	int i = 0;

	down_interruptible(&write_op_mutex);
	down_interruptible(&empty);
	
	write_index %= buffer_size;
	
	for(user_buffer_index = 0; user_buffer_index < number_of_chars_to_write; user_buffer_index++){
		if(buffer_empty_slots <= 0){
			break;
		}
		copy_from_user(&buffer[write_index][user_buffer_index], &user_buffer[user_buffer_index], 1);
	}
	
	++write_index;
	--buffer_empty_slots;
	
	up(&full);
	up(&write_op_mutex);
	
	return user_buffer_index;
}

static int fifo_release(struct inode* _inode, struct file* _file){
	--open_count;
	return 0;
}

void cleanup_module(){
	int _iter;
	for(_iter = 0; _iter < buffer_size; _iter++){
		kfree(buffer[_iter]);
	}
	misc_deregister(&fifo);
	printk(KERN_INFO "Device %s Unregistered!\n", device_name);
}


