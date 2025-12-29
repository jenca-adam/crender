SINGLETHREAD ?= 0
PROF ?= 0
DEBUG ?= 0
NUM_DOUBLE ?= 0
BUILD_DIR = ./build
LIB_BUILD_DIR = $(BUILD_DIR)/lib
INCLUDE_BUILD_DIR = $(BUILD_DIR)/include
SRC_DIR = ./src
INCLUDE_DIR = $(BUILD_DIR)/include
CC = gcc
CLANG_FORMAT = clang-format

CRENDER_CFLAGS = -Wall -Wextra -pedantic -O3 -march=native -ffast-math -funroll-loops
EXAMPLE_CFLAGS = -Wall -Wextra -pedantic -O3 
CRENDER_LDLIBS = -lm
EXAMPLE_LDLIBS = -lm -l:libcrender.a -lSDL2
CRENDER_DEFS = -DNO_BFCULL

INCLUDES = -I/usr/include 

CRENDER_SRC_DIR = $(SRC_DIR)/vec.c  $(SRC_DIR)/texture.c $(SRC_DIR)/obj.c $(SRC_DIR)/tri.c $(SRC_DIR)/scene.c
CRENDER_OBJ = $(CRENDER_SRC_DIR:.c=.o)
CRENDER_HDR = $(INCLUDE_DIR)/crender.h
CRENDER_HDR_IN = $(SRC_DIR)/crender.h
CRENDER_DYNLIB = $(LIB_BUILD_DIR)/libcrender.so
CRENDER_STLIB = $(LIB_BUILD_DIR)/libcrender.a
EXAMPLE_SDL = examples/sdl/renderer
EXAMPLE_SDL_SRC_DIR = examples/sdl/main.c examples/sdl/display.c
ifeq ($(SINGLETHREAD),1)
	CRENDER_DEFS += -DNO_MULTITHREAD
else
	CRENDER_LDLIBS += -fopenmp
	EXAMPLE_LDLIBS += -fopenmp
endif
ifeq ($(NUM_DOUBLE),1)
	CRENDER_DEFS += -DNUM_DOUBLE
endif
ifeq ($(PROF),1)
	CRENDER_CFLAGS += -pg
	EXAMPLE_CFLAGS += -pg
endif
ifeq ($(DEBUG),1)
	CRENDER_FLAGS += -g3
	EXAMPLE_FLAGS += -g3
endif

.PHONY: all clean remake example_sdl crender
all: crender example_sdl

crender: $(CRENDER_HDR) $(CRENDER_DYNLIB) $(CRENDER_STLIB)
	@cp $(CRENDER_HDR_IN) $(CRENDER_HDR)
	$(MAKE) oclean

$(CRENDER_HDR):
	@mkdir -p $(INCLUDE_DIR)
	$(CLANG_FORMAT) -i $(CRENDER_HDR_IN)
$(CRENDER_DYNLIB): $(CRENDER_OBJ)
	@mkdir -p $(LIB_BUILD_DIR)
	$(CC) -shared -fPIC $(CRENDER_OBJ) -o $@ $(CRENDER_LDLIBS)
$(CRENDER_STLIB): $(CRENDER_OBJ)
	@mkdir -p $(LIB_BUILD_DIR)
	$(AR) $(ARFLAGS) $@ $^
example_sdl: $(EXAMPLE_SDL)

$(EXAMPLE_SDL): $(EXAMPLE_SDL_SRC_DIR)
	$(CC) $(EXAMPLE_SDL_SRC_DIR) $(EXAMPLE_CFLAGS) -I$(INCLUDE_DIR) -I$(INCLUDES) -L$(LIB_BUILD_DIR) $(EXAMPLE_LDLIBS) -o $@

%.o: %.c
	$(CLANG_FORMAT) -i $<
	$(CC) $(CRENDER_CFLAGS) $(CRENDER_DEFS) $(INCLUDES) -fPIC -c $< -o $@ $(CRENDER_LDLIBS)

clean:
	rm -f $(CRENDER_DYNLIB) $(CRENDER_STLIB) $(EXAMPLE_SDL) $(CRENDER_OBJ) $(EXAMPLE_SDL_OBJ)
oclean:
	rm -f $(CRENDER_OBJ) $(EXAMPLE_SDL_OBJ)
remake: clean all

