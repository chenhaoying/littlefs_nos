TARGET = lfs_test
BUILD_DIR = build

C_SOURCES = $(wildcard ./*.c)

C_INCLUDES = -I.

CFLAGS = -std=c99 -Wall

all: $(TARGET)

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
# @$(info $(notdir $@))
	gcc -c $(CFLAGS) $< -o $@

$(TARGET): $(BUILD_DIR) $(OBJECTS) Makefile
	gcc $(OBJECTS) -o $@

$(BUILD_DIR):
	mkdir -p $@

clean: 
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)

rebuild: clean all
