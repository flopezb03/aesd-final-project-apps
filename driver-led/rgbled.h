#ifndef RGBLED_CHAR_DRIVER_H_
#define RGBLED_CHAR_DRIVER_H_

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/atomic.h>

struct rgbled_dev{
    struct cdev cdev;
    atomic_t opened;
    uint8_t color;
};

#endif