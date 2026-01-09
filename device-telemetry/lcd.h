#ifndef LCD_H

#define LCD_H

//  I2C Address
#define LCD_ADDR 0x27



void init_lcd(int fd);
void write_lcd(int fd, char* s1, char* s2);



#endif