WHAL_DIR = $(PWD)/../../

INCLUDE = -I$(WHAL_DIR)
CFLAGS = -Wall -Werror $(INCLUDE) -g3

# Test sources
TEST_SRC = test_main.c test_bitops.c test_dispatch.c

SOURCE = $(TEST_SRC)
OBJECTS = $(patsubst %.c,%.o,$(SOURCE))

TARGET = test_sim
TARGET_RT = test_sim_rt

all: $(TARGET) $(TARGET_RT)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(TARGET_RT): $(SOURCE)
	$(CC) $(CFLAGS) -DWHAL_RUNTIME_POLYMORPHISM -o $@ $(SOURCE)

.PHONY: run clean

run: $(TARGET) $(TARGET_RT)
	./$(TARGET)
	./$(TARGET_RT)

clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET_RT)
