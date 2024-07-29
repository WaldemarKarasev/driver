#ifndef IOCTL_FLAGS_H
#define IOCTL_FLAGS_H

#include "device.h"


#define WR_VALUE        _IOW('a', 'a', int32_t *)
#define RD_VALUE        _IOR('a', 'b', int32_t *)
#define LAST_WRITER     _IOR('a', 'c', struct process_info *)
#define LAST_READER     _IOR('a', 'd', struct process_info *)


#endif