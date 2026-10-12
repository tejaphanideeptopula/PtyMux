
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -D_GNU_SOURCE -Iinclude
LDFLAGS =

TARGET = ptymux

SOURCES = src/main.c \
          src/pty.c \
          src/session.c \
          src/terminal.c \
          src/multiplexer.c \
          src/commands.c \
          src/buffer.c \
          src/signal_handler.c \
          src/server.c \
          src/client.c

OBJECTS = $(SOURCES:src/%.c=build/%.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p bin
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f build/*.o $(TARGET)
