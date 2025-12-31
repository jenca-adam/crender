
SINGLETHREAD ?= 0
PROF ?= 0
DEBUG ?= 0
NUM_SIZE ?= float
O0 ?= 0
THREADUNSAFE ?= 0
EXPAND_DRAW_FACE_MACRO ?= 0
BUILD_DIR = ./build
LIB_BUILD_DIR = $(BUILD_DIR)/lib
INCLUDE_BUILD_DIR = $(BUILD_DIR)/include
SRC_DIR = ./src
INCLUDE_DIR = $(BUILD_DIR)/include
CC = gcc
CLANG_FORMAT = clang-format

CRENDER_CFLAGS = -Wall -Wextra -pedantic  -march=native -ffast-math -funroll-loops
EXAMPLE_CFLAGS = -Wall -Wextra -pedantic  
CRENDER_LDLIBS = -lm
EXAMPLE_LDLIBS = -lm -l:libcrender.a -lSDL2

INCLUDES = -I/usr/include 
CRENDER_UGLY_MACRO_DEFS = 
CRENDER_UGLY_MACRO_IN = $(SRC_DIR)/texture.c
CRENDER_EXPANDED_UGLY_MACRO = $(SRC_DIR)/texture_expanded.c
CRENDER_CFG_IN = $(SRC_DIR)/crender_cfg.h
CRENDER_CFG = $(INCLUDE_DIR)/crender_cfg.h
CRENDER_SRC = $(SRC_DIR)/vec.c  $(SRC_DIR)/obj.c $(SRC_DIR)/tri.c $(SRC_DIR)/scene.c $(SRC_DIR)/abi_tag.c
CRENDER_OBJ = $(CRENDER_SRC:.c=.o)
CRENDER_HDR = $(INCLUDE_DIR)/crender.h
CRENDER_HDR_IN = $(SRC_DIR)/crender.h
CRENDER_DYNLIB = $(LIB_BUILD_DIR)/libcrender.so
CRENDER_STLIB = $(LIB_BUILD_DIR)/libcrender.a
EXAMPLE_SDL = examples/sdl/renderer
EXAMPLE_SDL_SRC_DIR = examples/sdl/main.c examples/sdl/display.c

ifeq ($(O0), 1)
	CRENDER_CFLAGS += -O0
	EXAMPLE_CFLAGS += -O0
else
	CRENDER_CFLAGS += -O3
	EXAMPLE_CFLAGS += -O3
endif
ifeq ($(SINGLETHREAD),0)
	CRENDER_LDLIBS += -fopenmp
	EXAMPLE_LDLIBS += -fopenmp
else
	CRENDER_UGLY_MACRO_DEFS += -DCRENDER_CFG_NO_MULTITHREAD
endif
ifeq ($(PROF),1)
	CRENDER_CFLAGS += -pg
	EXAMPLE_CFLAGS += -pg
endif
ifeq ($(DEBUG),1)
	CRENDER_CFLAGS += -g3
	EXAMPLE_CFLAGS += -g3
endif
ifeq ($(EXPAND_DRAW_FACE_MACRO), 1)
	CRENDER_SRC += $(CRENDER_EXPANDED_UGLY_MACRO)
else
	CRENDER_SRC += $(CRENDER_UGLY_MACRO_IN)
endif
.PHONY: all clean remake example_sdl crender
all: crender example_sdl
_FORCE:
crender: $(CRENDER_CFG_IN) $(CRENDER_HDR) $(CRENDER_DYNLIB) $(CRENDER_STLIB)
	@cp $(CRENDER_HDR_IN) $(CRENDER_HDR)
	$(MAKE) oclean
$(CRENDER_CFG_IN): _FORCE
	echo "#ifndef _CRENDER_CFG_H" > $@
	echo "#define _CRENDER_CFG_H" >> $@
	echo "#define CR_CFG_NO_BFCULL 1" >> $@
ifeq ($(NUM_SIZE),double)
	echo "#define CR_CFG_NUM_DOUBLE 1" >> $@
endif
ifeq ($(SINGLETHREAD),1)
	echo "#define CR_CFG_NO_MULTITHREAD 1" >> $@
endif
ifeq ($(THREADUNSAFE),1)
	echo "#define CR_CFG_NO_LOCK 1" >> $@
endif
	echo "#endif" >> $@
	cp $@ $(CRENDER_CFG)
$(CRENDER_EXPANDED_UGLY_MACRO): $(CRENDER_UGLY_MACRO_IN)
	gcc -E -P -dD $(CRENDER_UGLY_MACRO_DEFS) $< -o $@
	clang-format -i $@
$(CRENDER_HDR): $(CRENDER_HDR_IN)
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

