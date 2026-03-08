#include "led.h"

#include <unistd.h>

void set_rgb_color(int fd, uint8_t code){
    uint8_t actual_color;

    read(fd, &actual_color, sizeof(actual_color));

    if(actual_color != code)
        write(fd, &code, sizeof(code));

}