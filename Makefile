# Requirements: (run apt install once if make fails)
# Note: build tools, SDL2, OpenGL, ptheread is for threading processes (multi cpu)

# For linesplus and songgen
# sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev

# `make clean` if you changed any source code between builds
# `make` will make linesplus and songgen (preferred method)
# `make -j$(nproc)` will builld using all processor cores (faster build time)

# `make linesplus` for just linesplus (requires a songgen build)
# `make songgen` for just songgen (this is standalone)

# Ignore me
# `make songview` broken software WIP (do not bother)
# For songview include some additional dependancies (do not bother)
# sudo apt install libglu1-mesa-dev libfftw3-dev

# Compiler and flags
CC = g++
CFLAGS = -Wall -O3 -Iinclude -std=c++17
LDFLAGS = -lSDL2 -lSDL2_image -lGL -pthread
SONGVIEW_LDFLAGS = -lSDL2 -lGL -lGLU -lfftw3

# Directories
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
SOURCES = $(filter-out $(SRC_DIR)/songgen.cpp $(SRC_DIR)/songview.cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
SONGVIEW_OBJ = $(OBJ_DIR)/songview.o
SONGGEN_OBJ = $(OBJ_DIR)/songgen.o
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)
EXEC = linesplus
SONGVIEW_EXEC = songview
SONGGEN_EXEC = songgen

# Default target: build linesplus and songgen
all: $(EXEC) $(SONGGEN_EXEC)

# Build linesplus executable
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) $(LDFLAGS)

# Build songgen executable
$(SONGGEN_EXEC): $(SONGGEN_OBJ)
	$(CC) $(SONGGEN_OBJ) -o $(SONGGEN_EXEC) $(LDFLAGS)

# Build songview executable (only when explicitly requested)
$(SONGVIEW_EXEC): $(SONGVIEW_OBJ)
	$(CC) $(SONGVIEW_OBJ) -o $(SONGVIEW_EXEC) $(SONGVIEW_LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean all artifacts
clean:
	rm -rf $(OBJ_DIR) $(EXEC) $(SONGVIEW_EXEC) $(SONGGEN_EXEC)

.PHONY: all clean