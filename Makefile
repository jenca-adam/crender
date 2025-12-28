
CC = gcc
CLANG_FORMAT = clang-format
CFLAGS = -g3 -Wall -Wextra -pedantic -O3 -march=native -ffast-math -funroll-loops -DNO_BFCULL
CLIBS = -lm -lSDL2 -fopenmp
INCLUDES = -I/usr/include 
SRC = src/main.c src/vec.c src/core.c src/color.c src/texture.c src/display.c src/obj.c src/tri.c src/scene.c
OBJ = $(SRC:.c=.o)
EXEC = renderer
all: $(EXEC)
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) -o $(EXEC) $(CLIBS)
%.o: %.c
	$(CLANG_FORMAT) $< -i
	$(CC) $(CFLAGS) $(INCLUDES)-c $< -o $@ $(CLIBS)
prof:	all
prof: 	CFLAGS += -pg
singlethread: $(EXEC)
singlethread: CFLAGS += -DNO_MULTITHREAD
clean:
	rm -f $(OBJ) $(EXEC)
remake: clean all
.PHONY: all clean prof remake win singlethread
