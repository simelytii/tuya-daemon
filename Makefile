TARGET = tuya-daemon

SDK_DIR = tuya-iot-core-sdk
SDK_BUILD_DIR = $(SDK_DIR)/build
SDK_LIB_DIR = $(SDK_BUILD_DIR)/lib

SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

CFLAGS = -Wall -Wextra \
         -Iinclude \
         -I$(SDK_DIR)/include \
         -I$(SDK_DIR)/interface \
         -I$(SDK_DIR)/utils

LDFLAGS = -L$(SDK_LIB_DIR) \
          -Wl,-rpath,$(SDK_LIB_DIR)

LIBS = -llink_core \
       -lmiddleware_implementation \
       -lplatform_port \
       -lutils_modules \
       -lcjson

.PHONY: all sdk clean

all: $(TARGET)

$(TARGET): sdk $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

sdk:
	cmake -S $(SDK_DIR) -B $(SDK_BUILD_DIR) -DBUILD_SHARED_LIBS=ON
	cmake --build $(SDK_BUILD_DIR)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)