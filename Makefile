CC = gcc
CFLAGS = -Wall -Wextra -I./include
SRC = src/main.c src/prompt.c src/parser.c src/executor.c src/builtins.c src/raw_input.c src/variables.c
OBJ = $(SRC:src/%.c=build/%.o)
TARGET = bin/main

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $(TARGET)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
	rm -rf build

.PHONY: all run clean
