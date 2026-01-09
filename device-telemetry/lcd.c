#include "lcd.h"

#include <stdint.h>
#include <unistd.h>
#include <string.h>


// Pin
#define LCD_BIT_ENABLE      0b00000100
#define LCD_BIT_BACKLIGHT   0b00001000

// Commands
#define LCD_CMD_CLEAR       0b00000001

#define LCD_CMD_RETURN      0b00000010

#define LCD_CMD_ENTRYMODE   0b00000100
#define LCD_CMD_ENTRY_INC       0b00000010
#define LCD_CMD_ENTRY_SHIFT     0b00000001

#define LCD_CMD_DISPLAY     0b00001000
#define LCD_CMD_DISPLAY_ON      0b00000100
#define LCD_CMD_DISPLAY_CURSOR  0b00000010
#define LCD_CMD_DISPLAY_BLINK   0b00000001

#define LCD_CMD_SHIFT       0b00010000
#define LCD_CMD_SHIFT_DISPLAY   0b00001000
#define LCD_CMD_SHIFT_RIGHT     0b00000100

#define LCD_CMD_FUNCTION    0b00100000
#define LCD_CMD_FUNCTION_8BITS  0b00010000
#define LCD_CMD_FUNCTION_2LINES 0b00001000

#define LCD_CMD_SET_ADDRESS 0b10000000





static uint8_t lcd_set_bit(uint8_t byte, uint8_t bit){
    return byte | bit;
}
static uint8_t lcd_unset_bit(uint8_t byte, uint8_t bit){
    return byte & ~bit;
}
static void lcd_write_device(int fd, uint8_t byte){
    write(fd, &byte, 1);
    usleep(1000);
}

static void lcd_write_nibble(int fd, uint8_t rs, uint8_t nibble){
    uint8_t byte = nibble;
    byte = lcd_set_bit(byte, LCD_BIT_BACKLIGHT);    // set backlight on
    byte = lcd_set_bit(byte, rs);                   // set rs

    byte = lcd_unset_bit(byte, LCD_BIT_ENABLE);     // set enable off
    lcd_write_device(fd, byte);

    byte = lcd_set_bit(byte, LCD_BIT_ENABLE);       // set enable on
    lcd_write_device(fd, byte);

    byte = lcd_unset_bit(byte, LCD_BIT_ENABLE);     // set enable off
    lcd_write_device(fd, byte);
}

static void lcd_write_byte(int fd, uint8_t rs, uint8_t data){
    lcd_write_nibble(fd, rs, data & 0b11110000);        // only high 4 bits
    lcd_write_nibble(fd, rs, (data << 4) & 0b11110000); // only low 4 bits
}





void init_lcd(int fd){
    uint8_t func = 0;

    //  Reset byte to 0
    write(fd, &func, 1);

    //  Set 4-bits mode
    func = LCD_CMD_FUNCTION | LCD_CMD_FUNCTION_8BITS;   // Interface data length set to 8 bits
    lcd_write_nibble(fd, 0, func); usleep(35000);
    lcd_write_nibble(fd, 0, func); usleep(35000);
    lcd_write_nibble(fd, 0, func); usleep(35000);
    func = LCD_CMD_FUNCTION;
    lcd_write_nibble(fd, 0, func); usleep(35000);

    //  Set 4-bits and 2 lines
    func = LCD_CMD_FUNCTION | LCD_CMD_FUNCTION_2LINES;
    lcd_write_byte(fd, 0, func);

    //  Clear Display
    func = LCD_CMD_CLEAR;
    lcd_write_byte(fd, 0, func);

    //  Entry mode
    func = LCD_CMD_ENTRYMODE | LCD_CMD_ENTRY_INC;
    lcd_write_byte(fd, 0, func);

    //  Display Control
    func = LCD_CMD_DISPLAY | LCD_CMD_DISPLAY_ON;
    lcd_write_byte(fd, 0, func);
}

void write_lcd(int fd, char* s1, char* s2){
    int s1_len, s2_len;

    s1_len = strlen(s1);
    s2_len = strlen(s2);

    //  Clear Display
    lcd_write_byte(fd, 0, LCD_CMD_CLEAR);

    for (int i = 0; i < s1_len; i++)
        lcd_write_byte(fd, 1, s1[i]);

    //  Go to first position of second line
    lcd_write_byte(fd, 0, LCD_CMD_SET_ADDRESS | 0x40);

    for (int i = 0; i < s2_len; i++)
        lcd_write_byte(fd, 1, s2[i]);
}