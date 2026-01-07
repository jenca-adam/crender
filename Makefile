all: nob
nob: nob.c
	gcc nob.c -o nob
build:
	./nob

.PHONY: all build
