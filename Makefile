# Compiler and flags
CC = g++
CFLAGS = -Wall -O3 -Iinclude -std=c++17
LDFLAGS = -lSDL2 -lSDL2_image -lGL -pthread

# This also builds songgen
# Build everything: make
# Build only songgen: make songgen
# Build only linesplus: make linesplus
# Clean all: make clean
# Run songgen: ./songgen

# Directories
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj/linesplus
SOURCES = $(filter-out $(SRC_DIR)/songgen.cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)
EXEC = linesplus

# Default target: build linesplus and optionally songgen
all: $(EXEC) songgen

# Build linesplus executable
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) $(LDFLAGS)

# Compile source files to object files for linesplus
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build songgen using Makefile.soundgen
songgen:
	$(MAKE) -f Makefile.songgen

# Clean both linesplus and songgen artifacts
clean:
	rm -rf $(OBJ_DIR) $(EXEC)
	$(MAKE) -f Makefile.songgen clean

.PHONY: all clean songgen