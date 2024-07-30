#include "device.h"

int fake_device_read(struct fake_device* device, struct file* filp, const char* buf_store_data, size_t buf_count)
{
    int ret;
    if(filp->f_flags & O_NONBLOCK)
    {
        if (!mutex_trylock(&virtual_device.lock_))
        {
            return -EAGAIN;
        }
        virtual_device.last_reader_.pid_ = current_uid();
        ktime_get_real_ts64(&virtual_device.last_reader_.time_);
        ret = read_from_circular_buffer(&virtual_device.buffer_, buf_store_data, buf_count);
    }
    else
    {
        mutex_lock(&virtual_device.lock_);
        // Critical section start

        virtual_device.last_reader_.pid_ = current_uid();
        ktime_get_real_ts64(&virtual_device.last_reader_.time_);
        ret = read_from_circular_buffer(&virtual_device.buffer_, buf_store_data, buf_count);

        // end
    }

    // If we are here, we must unlock mutex 
    mutex_unlock(&virtual_device.lock_);

    return ret;
}


int fake_device_write(struct fake_device* device, struct file* filp, const char* buf_store_data, size_t buf_count)
{
    int ret;
    if(filp->f_flags & O_NONBLOCK)
    {
        if (!mutex_trylock(&virtual_device.lock_))
        {
            // return if device is locked
            return -EAGAIN;
        }
        
        virtual_device.last_writer_.pid_ = current_uid();
        ktime_get_real_ts64(&virtual_device.last_writer_.time_);
        ret = write_to_circular_buffer(&virtual_device.buffer_, buf_store_data, buf_count);
    }
    else
    {
        mutex_lock(&virtual_device.lock_);
        // Critical section start

        virtual_device.last_reader_.pid_ = current_uid();
        ktime_get_real_ts64(&virtual_device.last_writer_.time_);
        ret = write_to_circular_buffer(&virtual_device.buffer_, buf_store_data, buf_count);

        // end
    }

    // If we are here, we must unlock mutex 
    mutex_unlock(&virtual_device.lock_);

    return ret;
}