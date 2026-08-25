CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c17 -g
INCLUDES = -Iinclude

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = scheduler

.PHONY: all clean debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: all

clean:
	rm -f src/*.o $(TARGET)
