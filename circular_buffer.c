#include "circular_buffer.h"

// Write/Read data from/to user
#include <linux/uaccess.h>

// include for memory managment
#include <linux/slab.h>


#include <linux/kernel.h>
#include <linux/module.h>

// Module info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Karasev Vladimir");
MODULE_DESCRIPTION("Test task");


// Service functions for circular buffer
// Check if the queue is full
int is_full(struct circular_buffer* buffer);

// Check if the queue is empty
int is_empty(struct circular_buffer* buffer);

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
    buffer->read_index_ = 0;
    buffer->write_index_ = 0;

    // Setting size for buffer
    buffer->size_ = size;
    buffer->data_size_ = 0;

    printk("Buffer allocated");
    return 0;
}

// Destroy buffer. Free allocated memory
void destroy_circular_buffer(struct circular_buffer* buffer)
{
    kfree(buffer->data_);
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
    if (size_to_write == 0)
    {
        return 0;
    }

    int writable_size = size_to_write;
    const char* user_ptr = src;

    // overflow case
    if (writable_size > buffer->size_)
    {
        user_ptr = src + (writable_size - buffer->size_);
        writable_size = buffer->size_;
    }

    int reset_head = 0;
    // writing to buffer
    // case 1: buffer->data won't be full after write into it
    if (buffer->write_index_ + writable_size < buffer->size_)
    {
        copy_from_user(buffer->data_, user_ptr, writable_size); 

        if ((buffer->write_index_ < buffer->read_index_) &&
            (buffer->write_index_ + writable_size >= buffer->size_))
            {
                reset_head = 1;
            }

        buffer->write_index_ += writable_size;       
    }
    else // case 2: buffer->data will be overflowed after writing into it
    {
        int remain_size = buffer->size_ - buffer->write_index_ - 1; // remaining size
        copy_from_user(&buffer->data_[buffer->write_index_ + 1], user_ptr, remain_size);

        int cover_size = writable_size - remain_size; // size of the data to be covered from begining
        copy_from_user(buffer->data_, user_ptr + remain_size, cover_size);

        if (buffer->write_index_ < buffer->read_index_)
        {
            reset_head = 1;
        }
        else
        {
            if (cover_size > buffer->read_index_)
            {
                reset_head = 1;
            }

            buffer->write_index_ = cover_size - 1;
        }

        if (reset_head)
        {
            if (buffer->write_index_ + 1 < buffer->size_)
            {
                buffer->read_index_ = buffer->write_index_ + 1;
            }
            else
            {
                buffer->read_index_ = 0;
            }

            buffer->data_size_ = buffer->size_;
        }
        else
        {
            if (buffer->write_index_ >= buffer->read_index_)
            {
                buffer->data_size_ = buffer->write_index_ - buffer->read_index_ + 1;
            }
            else
            {
                buffer->data_size_ = buffer->size_ - (buffer->read_index_ - buffer->write_index_ - 1);
            }
        }
    }

    return writable_size;
}

// Removing an element
int read_from_circular_buffer(struct circular_buffer* buffer, char* dest, int size_to_read)
{
    int reset_head = 1;

    if (buffer->data_size_ == 0 || size_to_read == 0)
    {
        return 0;
    }

    int read_size = size_to_read;

    if (buffer->data_size_ < read_size)
    {
        read_size = buffer->data_size_;
    }

    char* user_ptr = dest;
    if (buffer->read_index_ <= buffer->write_index_)
    {
        copy_to_user(user_ptr, &buffer->data_[buffer->read_index_], read_size); 
        if (reset_head)
        {
            buffer->read_index_ += read_size;
            if (buffer->read_index_ > buffer->write_index_)
            {
                buffer->read_index_ = 0;
                buffer->write_index_ = 0;
            }
        }
    }
    else
    {
        if (buffer->read_index_ + read_size < buffer->size_)
        {
            copy_to_user(user_ptr, &buffer->data_[buffer->read_index_], read_size);

            if (reset_head)
            {
                buffer->read_index_ += read_size;
                if (buffer->read_index_ == buffer->size_)
                {
                    buffer->read_index_ = 0;
                }
            }
        }
        else
        {
            int len1 = buffer->size_ - buffer->read_index_;
            copy_to_user(user_ptr, &buffer->data_[buffer->read_index_], len1);

            int len2 = read_size - len1;
            copy_to_user(user_ptr + len1, buffer->data_ ,len2);

            if (reset_head)
            {
                buffer->read_index_ = len2;
                if (buffer->read_index_ > buffer->write_index_)
                {
                    buffer->read_index_ = 0;
                    buffer->write_index_ = 0;
                }
            }
        }
    }

    if (reset_head)
    {
        buffer->data_size_ -= read_size;
    }

    return read_size;

}