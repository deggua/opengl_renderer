PROJ_NAME = opengl
EXE_EXT = exe

# compiler/linker
CC = clang
LD = lld-link
OBJCOPY = bin2coff # using this tool on windows, can't get objcopy to work

# flags
CC_FLAGS_FILE = compile_flags.txt
CC_FLAGS = $(shell type $(CC_FLAGS_FILE))

CC_FLAGS_DEBUG_MODE = -g3 -O0 -fuse-ld=lld

CC_FLAGS_RELEASE_MODE = -g3 -Ofast -fuse-ld=lld -flto -Wl,/LTCG
CC_FLAGS_RELEASE_MODE += -funsafe-math-optimizations -fno-math-errno
CC_FLAGS_RELEASE_MODE += -DNDEBUG

CC_FLAGS_DEBUG = $(CC_FLAGS_DEBUG_MODE) $(CC_FLAGS)
CC_FLAGS_RELEASE = $(CC_FLAGS_RELEASE_MODE) $(CC_FLAGS)
CC_FLAGS_SANITIZE = $(CC_FLAGS_DEBUG) -fsanitize=address
CC_FLAGS_PROFILE = $(CC_FLAGS_RELEASE) -pg -a

# directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# normal source files
SRCS = $(SRC_DIR)/main.cpp
SRCS += $(wildcard $(SRC_DIR)/common/*.cpp)

# platform source files
# SRCS += $(wildcard $(SRC_DIR)/platform/windows/*.c)

# embedded data
DATA = $(wildcard $(SRC_DIR)/shaders/*.glsl)

OBJS = $(SRCS:src/%.cpp=obj/%.o)
OBJS += $(DATA:src/%.glsl=obj/%.o)

# output file names
DEBUG_FNAME 	:= $(PROJ_NAME)-dbg.$(EXE_EXT)
RELEASE_FNAME 	:= $(PROJ_NAME).$(EXE_EXT)
SANITIZE_FNAME 	:= $(PROJ_NAME)-asan.$(EXE_EXT)
PROFILE_FNAME 	:= $(PROJ_NAME)-gprof.$(EXE_EXT)

all: debug release

debug: $(BIN_DIR)/$(DEBUG_FNAME)

release: $(BIN_DIR)/$(RELEASE_FNAME)

$(BIN_DIR)/$(DEBUG_FNAME): $(OBJS)
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(CC) -o $(BIN_DIR)/$(DEBUG_FNAME) $(CC_FLAGS_DEBUG) $(OBJS)

$(BIN_DIR)/$(RELEASE_FNAME): $(OBJS)
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(CC) -o $(BIN_DIR)/$(RELEASE_FNAME) $(CC_FLAGS_RELEASE) $(OBJS)

obj/%.o: src/%.cpp
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(CC) -o $@ $(CC_FLAGS_RELEASE) -c $<

obj/%.o: src/%.glsl
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(OBJCOPY) $< $@ $(basename $(<F)) 64bit

sanitize:
	$(shell if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)")
	$(CC) -o $(BIN_DIR)/$(SANITIZE_FNAME) $(CC_FLAGS_SANITIZE) $(SRCS)

profile:
	$(shell if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)")
	$(CC) -o $(BIN_DIR)/$(PROFILE_FNAME) $(CC_FLAGS_PROFILE) $(SRCS)

clean:
	rmdir /s /q $(BIN_DIR)
	rmdir /s /q $(OBJ_DIR)
