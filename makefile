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

CC_FLAGS_DEBUG = $(CC_FLAGS_DEBUG_MODE) $(CC_FLAGS)
CC_FLAGS_RELEASE = $(CC_FLAGS_RELEASE_MODE) $(CC_FLAGS)
CC_FLAGS_SANITIZE = $(CC_FLAGS_DEBUG) -fsanitize=address
CC_FLAGS_PROFILE = $(CC_FLAGS_RELEASE) -pg -a

# directories
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build

# normal source files
SRCS = $(SRC_DIR)/main.cpp
SRCS += $(wildcard $(SRC_DIR)/common/*.cpp)
SRCS += $(wildcard $(SRC_DIR)/utils/*.cpp)
SRCS += $(wildcard $(SRC_DIR)/gfx/*.cpp)
SRCS += $(wildcard $(SRC_DIR)/math/*.cpp)

SRCS += $(wildcard $(SRC_DIR)/dependencies/imgui/*.cpp)
SRCS += $(SRC_DIR)/dependencies/imgui/backends/imgui_impl_glfw.cpp
SRCS += $(SRC_DIR)/dependencies/imgui/backends/imgui_impl_opengl3.cpp

# platform source files
# SRCS += $(wildcard $(SRC_DIR)/platform/windows/*.c)

# embedded data
DATA = $(wildcard $(SRC_DIR)/shaders/*.glsl)

DEBUG_OBJS = $(SRCS:src/%.cpp=$(BUILD_DIR)/debug/%.o)
DEBUG_OBJS += $(DATA:src/%.glsl=$(BUILD_DIR)/debug/%.o)
DEBUG_DEPS = $(DEBUG_OBJS:%.o=%.d)

RELEASE_OBJS = $(SRCS:src/%.cpp=$(BUILD_DIR)/release/%.o)
RELEASE_OBJS += $(DATA:src/%.glsl=$(BUILD_DIR)/release/%.o)
RELEASE_DEPS = $(RELEASE_OBJS:%.o=%.d)

# output file names
DEBUG_FNAME 	:= debug.$(EXE_EXT)
RELEASE_FNAME 	:= release.$(EXE_EXT)
SANITIZE_FNAME 	:= asan.$(EXE_EXT)
PROFILE_FNAME 	:= gprof.$(EXE_EXT)

# TODO: these don't work anymore
#sanitize:
#	$(shell if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)")
#	$(CC) -o $(BIN_DIR)/$(SANITIZE_FNAME) $(CC_FLAGS_SANITIZE) $(SRCS)

#profile:
#	$(shell if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)")
#	$(CC) -o $(BIN_DIR)/$(PROFILE_FNAME) $(CC_FLAGS_PROFILE) $(SRCS)

all: debug release

release: $(BIN_DIR)/$(RELEASE_FNAME)

$(BIN_DIR)/$(RELEASE_FNAME): $(RELEASE_OBJS)
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(CC) -o $(BIN_DIR)/$(RELEASE_FNAME) $(CC_FLAGS_RELEASE) $(RELEASE_OBJS)

-include $(RELEASE_DEPS)

$(BUILD_DIR)/release/%.o: $(SRC_DIR)/%.cpp
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(CC) -o $@ $(CC_FLAGS_RELEASE) -c $< -MMD

$(BUILD_DIR)/release/%.o: $(SRC_DIR)/%.glsl
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(OBJCOPY) $< $@ $(basename $(<F))_file 64bit

debug: $(BIN_DIR)/$(DEBUG_FNAME)

$(BIN_DIR)/$(DEBUG_FNAME): $(DEBUG_OBJS)
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(CC) -o $(BIN_DIR)/$(DEBUG_FNAME) $(CC_FLAGS_DEBUG) $(DEBUG_OBJS)

-include $(DEBUG_DEPS)

$(BUILD_DIR)/debug/%.o: $(SRC_DIR)/%.cpp
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(CC) -o $@ $(CC_FLAGS_DEBUG) -c $< -MMD

$(BUILD_DIR)/debug/%.o: $(SRC_DIR)/%.glsl
	$(shell if not exist "$(@D)" mkdir "$(@D)")
	$(OBJCOPY) $< $@ $(basename $(<F))_file 64bit

.PHONY: clean
clean:
	$(shell if exist "$(BIN_DIR)" rmdir /s /q "$(BIN_DIR)")
	$(shell if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)")
