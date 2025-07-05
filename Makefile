
CC = gcc
CLANG_FORMAT = clang-format
CFLAGS = -g3 -Wall -Wextra -pedantic -O4
CLIBS = -lm -lSDL2 -fopenmp
SRC = src/main.c src/vec.c src/core.c src/color.c src/texture.c src/display.c src/obj.c src/tri.c
OBJ = $(SRC:.c=.o)
EXEC = renderer
all: $(EXEC)
$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(CLIBS)
%.o: %.c
	$(CLANG_FORMAT) $< -i
	$(CC) $(CFLAGS) -c $< -o $@ $(CLIBS)
clean:
	rm -f $(OBJ) $(EXEC)
.PHONY: all clean
