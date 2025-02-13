# compiler
CC = gcc

# compile options
CFLAGS = -Wall -Wextra -O2 -Iinclude

# link options
LDFLAGS = -Llib -lm

# source files (all files in src directory)
SRC = $(wildcard src/*.c)

# object files
OBJ = $(patsubst)

# executable file
TARGET = bin/program

# rule definition
all: $(TARGET)

# compile rule
$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

# compile *.c to *.o
obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# clean up
clean:
	rm -rf bin/ obj/

# execute
run: $(TARGET)
	./$(TARGET)

# add compile options if debug build
debug: CFLAGS += -g -O0
debug: clean all