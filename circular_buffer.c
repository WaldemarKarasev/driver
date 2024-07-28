#include "circular_buffer.h"

// Write/Read data from/to user
#include <linux/uaccess.h>

// include for memory managment
#include <linux/slab.h>
#include <string.h>

// Creating circular buffer. Allocating memory
int create_circular_buffer(struct circular_buffer* buffer, int size)
{
    buffer->data_ = kmalloc(size, GFP_KERNEL);
    if (buffer->data_ == NULL)
    {
        printk("Out of memory");
        return -1;
    }

    // Seting "empty" state for buffer
    buffer->read_index_ = -1;
    buffer->write_index_ = -1;

    // Setting size for buffer
    buffer->size_ = size;

    printk("Buffer allocated");
    return 0;
}

// Destroy buffer. Free allocated memory
void destroy_circular_buffer(struct circular_buffer* buffer)
{
    kfree(buffer->data);
}


// Check if the queue is full
int is_full(struct circular_buffer* buffer)
{
    if ((buffer->read_index_ == buffer->write_index_ + 1) || (buffer->read_index_ == 0 && buffer->write_index_ == buffer->size_ - 1))
    {
        return 1;
    }    

    return 0;
}

// Check if the queue is empty
int is_empty(struct circular_buffer* buffer)
{
    if (buffer->read_index_ == -1)
    {
        return 1;
    }

    return 0;
}

// Adding an element
// Adding an element
int write_to_circular_buffer(struct circular_buffer* buffer, const char* src, int size_to_write)
{
    int size_to_copy;
    int not_copied;
    int delta;

    if (is_full(buffer))
    {
        return -1;
        printk("Buffer is full");
    }
    else
    {
        if (buffer->write_index_ == -1)
        {
            buffer->write_index_ = 0;
        }
        
        size_to_copy = min(size_to_write, buffer->size);

        not_copied = copy_from_user(buffer->data_, src, size_to_copy);

        delta = size_to_copy - not_copied;

        printk("Data is written to buffer");
    }

    return delta;
}

// Removing an element
int read_from_circular_buffer(struct circular_buffer* buffer, char* dest, int size_to_read)
{

}