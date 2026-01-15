CC = gcc
CFLAGS = -Wall -Wextra -I./include
SRC = src/main.c src/prompt.c src/parser.c src/executor.c src/builtins.c
OBJ = $(SRC:.c=.o)
TARGET = bin/main

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all run clean
