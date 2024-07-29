#ifndef DEVICE_H
#define DEVICE_H

#include <linux/kernel.h>
#include <linux/init.h>
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

#include <linux/semaphore.h>

// Mutex include
#include <linux/mutex.h>

// For saving last reader and writer
#include <linux/cred.h>

// circular_buffer includes
#include "circular_buffer.h"

struct process_info
{
    kuid_t pid_;                           // process pid
};

struct fake_device 
{
    struct circular_buffer buffer_;
    struct semaphore sem_;              // Constraining a number of procceses for enable to open the device 
    struct mutex lock_;                 // Mutual locking write/read operations
    struct process_info last_writer_;   // Last writer process info struct
    struct process_info last_reader_;   // Last reader process info struct
} virtual_device;

// fake device read
int fake_device_read(struct fake_device* device, struct file* filp, const char* buf_store_data, size_t buf_count);

// fake device write
int fake_device_write(struct fake_device* device, struct file* filp, const char* buf_store_data, size_t buf_count);

#endif