#include "led.h"

#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define GPIO_RED    21
#define GPIO_GREEN  20
#define GPIO_BLUE   16


void set_rgb_color(int fd, int code){

    struct gpiohandle_request req = {0};
    req.lines = 3;
    req.lineoffsets[0] = GPIO_RED;
    req.lineoffsets[1] = GPIO_GREEN;
    req.lineoffsets[2] = GPIO_BLUE;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    strcpy(req.consumer_label, "rgb_led");

    ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);

    struct gpiohandle_data data = {0};

    data.values[0] = (code >> 0) & 1; // R
    data.values[1] = (code >> 1) & 1; // G
    data.values[2] = (code >> 2) & 1; // B

    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);

    close(req.fd);
}