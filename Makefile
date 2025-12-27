
CC = gcc
CLANG_FORMAT = clang-format
CFLAGS = -g3 -Wall -Wextra -pedantic -O3 -march=native -ffast-math -funroll-loops
CLIBS = -lm -lSDL2 -fopenmp
INCLUDES = -I/usr/include 
SRC = src/main.c src/vec.c src/core.c src/color.c src/texture.c src/display.c src/obj.c src/tri.c
OBJ = $(SRC:.c=.o)
EXEC = renderer
all: $(EXEC)
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) -o $(EXEC) $(CLIBS)
%.o: %.c
	$(CLANG_FORMAT) $< -i
	$(CC) $(CFLAGS) $(INCLUDES)-c $< -o $@ $(CLIBS)
prof:	$(EXEC)
prof: 	CFLAGS += -pg
win: $(EXEC)
clean:
	rm -f $(OBJ) $(EXEC)
remake: clean all
.PHONY: all clean prof remake win
