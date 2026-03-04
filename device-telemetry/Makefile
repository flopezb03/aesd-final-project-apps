CC      ?= $(CROSS_COMPILE)gcc
CFLAGS  ?= -Wall -Wextra -g # -DBMP280_SPI for SPI mode in bmp280 sensor
LDFLAGS ?= 

TARGET  := telemetry-app
SRCS    := main.c bmp280.c bmp280spi.c lcd.c led.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean artifacts
clean:
	rm -f $(TARGET) $(OBJS)
