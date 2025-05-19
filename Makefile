# Compiler and flags
CC = g++
CFLAGS = -Wall -O3 -Iinclude -std=c++17
LDFLAGS = -lSDL2 -lSDL2_image -lGL -pthread
SONGVIEW_LDFLAGS = -lSDL2 -lGL -lGLU -lfftw3

# This also builds songgen and songview
# Build everything: make
# Build only songgen: make songgen
# Build only linesplus: make linesplus
# Build only songview: make songview
# Clean all: make clean
# Run songgen: ./songgen
# Run songview: ./songview <filename>.song
# Run linesplus: ./linesplus

# Directories
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj/linesplus
SOURCES = $(filter-out $(SRC_DIR)/songgen.cpp $(SRC_DIR)/songview.cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
SONGVIEW_OBJ = $(OBJ_DIR)/songview.o
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)
EXEC = linesplus
SONGVIEW_EXEC = songview

# Default target: build linesplus, songview, and songgen
all: $(EXEC) $(SONGVIEW_EXEC) songgen

# Build linesplus executable
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) $(LDFLAGS)

# Build songview executable
$(SONGVIEW_EXEC): $(SONGVIEW_OBJ)
	$(CC) $(SONGVIEW_OBJ) -o $(SONGVIEW_EXEC) $(SONGVIEW_LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build songgen using Makefile.songgen
songgen:
	$(MAKE) -f Makefile.songgen

# Clean linesplus, songview, and songgen artifacts
clean:
	rm -rf $(OBJ_DIR) $(EXEC) $(SONGVIEW_EXEC)
	$(MAKE) -f Makefile.songgen clean

.PHONY: all clean songgen linesplus songview