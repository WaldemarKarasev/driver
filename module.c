#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

#include <linux/moduleparam.h>

// Mutex include
#include <linux/mutex.h>

// IOCTL includes
#include <linux/ioctl.h>
#include <linux/fcntl.h>

// For saving last reader and writer
#include <linux/cred.h>

// include for O_NONBLOCK
#include <linux/fcntl.h>
// circular_buffer includes
#include "circular_buffer.h"

// ioctl custom flags include  
#include "ioctl_custom_flags.h"


// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Karasev Vladimir");
MODULE_DESCRIPTION("Test task");

// 1. Create a structure for our fake device
static int buffer_size = 100; // default size of the buffer will be 100 bytes
// Setting buffer size from command line
module_param(buffer_size, int, 0000);

struct fake_device {
    circular_buffer buffer_;
    struct semaphore sem_;
    struct mutex lock_;
    int last_writer_pid_;
    int last_reader_pid_;
} virtual_device;

// 2. To later register fake device we need a cdev obj and some other variables
struct cdev* my_char_dev;   // ptr for our fake device
int major_number;           // wiil store out major number - extrated from dev_T using macro - mknod /directory/file c major minor
int minor_number;           // so declaring variables all over the pass in our module functions eats up the stack very fast

int ret;
dev_t dev_num;              // wiil hold major number that kernel gives us
                            // name -> appears in /proc/deices

#define DEVICE_NAME         "virtual_char_device"

// 7. called on device_file open
//      inode reference to the file on disk
//      and contains information about that file
//      struct file is represent an abstract open file
int device_open(struct inode* inode, struct file* filp)
{
    // Only allow one process to open this device ny using a semaphore as mutual exclusive lock-mutex
    if (down_interruptible(&virtual_device.sem_) != 0)
    {
        printk(KERN_ALERT "could not lock device during open");
        return -1;
    }

    printk(KERN_INFO "opened device");
    return 0;
}


// 8. called when user wants to get the information from device
ssize_t device_read(struct file* filp, char* buf_store_data, size_t buf_count, loff_t* cur_offset)
{
    // take data from kernel space(device) to user space(process)
    // copy_to_user(dest, source, sizetotransfer)
    printk(KERN_INFO "Reading from device");


    if(filp->f_flags & O_NONBLOCK)
    {
        if (!mutex_try_lock(&virtual_device.lock_))
        {
            return -EAGAIN;
        }
        virtual_device.last_reader_pid_ = current_uid();
        ret = read_from_circular_buffer(&virtual_device.buffer, buf_store_data, buf_count);
    }
    else
    {
        mutex_lock(&virtual_device.lock_);
        // Critical section start

        virtual_device.last_reader_pid_ = current_uid();
        ret = read_from_circular_buffer(&virtual_device.buffer, buf_store_data, buf_count);

        // end
    }

    // If we are here, we must unlock mutex 
    mutex_unlock(&virtual_device.lock_);
    return ret;
}

// 9. called when user wants to send information to the device
ssize_t device_write(struct file* filp, const char* buf_store_data, size_t buf_count, loff_t* curr_offset)
{
    // send data to the kernel
    // copy_from_user(dest, source, count)

    printk(KERN_INFO "writing to device");

    if(filp->f_flags & O_NONBLOCK)
    {
        if (!mutex_try_lock(&virtual_device.lock_))
        {
            // return if device is locked
            return -EAGAIN;
        }
        
        virtual_device.last_writer_pid_ = current_uid();
        ret = write_to_circular_buffer(&virtual_device.buffer, buf_store_data, buf_count);
    }
    else
    {
        mutex_lock(&virtual_device.lock_);
        // Critical section start

        virtual_device.last_reader_pid_ = current_uid();
        ret = write_to_circular_buffer(&virtual_device.buffer, buf_store_data, buf_count);

        // end
    }

    // If we are here, we must unlock mutex 
    mutex_unlock(&virtual_device.lock_);
    
    return ret;
}

// 10. called upon user close
int device_close(struct inode* inode, struct file* filp)
{
    // by calling up, which is opposite of down for semaphore, we release the mutex that we obtained at device open
    // this has the effect of allowing other process to use the device now
    up(&virtual_device.sem);
    printk(KERN_INFO "closed device");
    return 0;
}

// 6. Tell the kernel which functions to call when user operates on out device file 
struct file_operations fops = {
    .owner = THIS_MODULE,       // prevent unloading of this module when operations are in use
    .open  = device_open,         // points to the method to call when opening the device
    .release = device_close,     // points to the method to call when closing the device 
    .write = device_write,       // points to the method to call when writing to the device 
    .read  = device_read         // points to the method to call when reading from device
};

static int driver_entry(void)
{
    // 3. Register out device with the system: a two step process
    // step 1: use dynamic allocation to assign out device
    //  a major number -- alloc_chrdev_region(dev_t*, minor, count, name);
    int ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
    {
        printk(KERN_ALERT "pingvinus: failed to allocate a major number");
        return ret; // propagate error
    }

    major_number = MAJOR(dev_num); // extract the major number and return it
    printk(KERN_INFO "major number is: %d", major_number);
    printk(KERN_INFO "use mknod /dev/%s c &d 0\" for device file", DEVICE_NAME, major_number); // dmesg

    // step 2:
    my_char_dev = cdev_alloc(); // create out dev structure, initialized out dev
    my_char_dev->ops = &fops;   // struct file_operations
    my_char_dev->owner = THIS_MODULE;
    // now that we created cdev, we have to add it to the kernel
    // int cdev_add(struct cdev* dev, dev_t num, unsigned int count);
    ret = cdev_add(my_char_dev, dev_num, 1);
    if (ret < 0)
    {
        printk(KERN_ALERT "unable to add cdev to kernel");
        return ret;
    }

    // 4. Initialize our semaphore
    sema_init(&virtual_device.sem_, 2); // initial value of one

    // 5. Initialize circular buffer
    ret = create_circular_buffer(&virtual_device.buffer_, buffer_size);
    if (ret < 0)
    {
        printk("Error in creating circular buffer for device");
        return ret;
    }

    // Initialize mutex
    mutex_init(&virtual_device.lock_);

    return 0;
}

static void driver_exit(void)
{
    // 5. unregister everything in reverse order
    // step 1:
    cdev_del(my_char_dev);

    // step 2:
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_ALERT "unloaded module");
}


int init_mod(void) {
    pr_info("Hello world\n");
    return 0;
}

void cleanup_mod(void) {
    pr_info("Goodbye world\n");
}


// Inform the kernel where to start and stop out module/driver
module_init(driver_entry);
module_exit(driver_exit);


// Module info
MODULE_LICENSE("GPL");