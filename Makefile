CC = gcc

SDK_DIR = $(HOME)/Documents/tuya-iot-core-sdk

CFLAGS = -Wall -Wextra -std=c99 -g \
    -Iinclude \
    -I/home/studentas/Documents/tuya-iot-core-sdk/include \
    -I/home/studentas/Documents/tuya-iot-core-sdk/interface \
    -I/home/studentas/Documents/tuya-iot-core-sdk/utils

LDFLAGS = -L$(SDK_DIR)/build/lib \
	-Wl,-rpath,$(SDK_DIR)/build/lib

LIBS = -llink_core \
	-lmiddleware_implementation \
	-lplatform_port \
	-lutils_modules

TARGET = tuya-daemon

SRC = src/main.c \
      src/daemon.c \
      src/system_info.c \
      src/network_info.c \
      src/cpu_info.c \
	  src/action.o

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)