#ifndef RGBLED_CHAR_DRIVER_H_
#define RGBLED_CHAR_DRIVER_H_

#include <linux/types.h>
#include <linux/cdev.h>

struct rgbled_dev{
    struct cdev cdev;
    struct mutex lock;
    uint8_t color;
};

#endif