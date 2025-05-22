# Makefile for building linesplus and songgen
#
# We do not install so we do not need a password.
#
# HOW TO USE:
# - Run 'make' to build linesplus and songgen (easiest way).
# - Run 'make -j$(nproc)' to build faster using all your computer's power.
# - Run 'make clean' if you changed any code before building again.
# - Run 'make linesplus' to build only linesplus (needs songgen built first).
# - Run 'make songgen' to build only songgen (works on its own).
# - Run 'make clean' to delete all built files and start fresh.
#
# AFTER BUILDING:
# - Run './linesplus' to play linesplus. (or use icon)
# - Run './songgen' for help
# - Run './songgen rock' to create a song
# - Run './songgen song1.song' to play song1
#
# REQUIREMENTS: (you may already have them installed)
# Run this command ONCE if the build fails:
#    NOTE: 'sudo' will ask for the password. Ask an adult for help if needed!
#	 To install software on Linux it is common to require a password.
#
#      sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev
#
# build-essential - programmer tools
# libsdl2-dev - SDL2 for controller input
# libsdl2-image-dev - SDL2 for pictures
# libgl1-mesa-dev - OpenGL to draw to the screen
# libpthread-stubs0-dev - Lets code create more threads to distribute workload
#
#
# TROUBLESHOOTING:
# - If the build fails, it might mean a library is missing. The error messages below will tell you what to install.
# - If you see a password prompt with 'sudo', ask an adult to help.
# - If you get stuck, ask an adult or a friend who knows coding!

# Compiler and flags (these tell the computer how to build the programs)
CC = g++
CFLAGS = -Wall -O3 -Iinclude -std=c++17
LDFLAGS = -lSDL2 -lSDL2_image -lGL -pthread
SONGVIEW_LDFLAGS = -lSDL2 -lGL -lGLU -lfftw3

# Directories (where the code and built files live)
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
	@echo "*** Successfully built linesplus and songgen! ***"
	@echo "*** Run './linesplus' to play or './songgen' to create a song. ***"

# Build linesplus executable
$(EXEC): $(OBJECTS)
	@echo "Building linesplus..."
	@$(CC) $(OBJECTS) -o $(EXEC) $(LDFLAGS) || ( \
		echo "*** Failed! Building linesplus failed. You might be missing some libraries. ***"; \
		echo "Try running this command (ask an adult for help with 'sudo'):"; \
		echo "  sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev"; \
		echo "Then run 'make' again."; \
		exit 1; \
	)
	@echo "*** linesplus built successfully! Run './linesplus' to play. ***"

# Build songgen executable
$(SONGGEN_EXEC): $(SONGGEN_OBJ)
	@echo "Building songgen..."
	@$(CC) $(SONGGEN_OBJ) -o $(SONGGEN_EXEC) $(LDFLAGS) || ( \
		echo "*** Building songgen failed. You might be missing some libraries. ***"; \
		echo "Try running this command (ask an adult for help with 'sudo'):"; \
		echo "  sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev"; \
		echo "Then run 'make songgen' again."; \
		exit 1; \
	)
	@echo "*** songgen built successfully! Run './songgen' to create a song. ***"

# Build songview executable (only when explicitly requested)
$(SONGVIEW_EXEC): $(SONGVIEW_OBJ)
	@echo "Building songview (warning: it's broken and might not work)..."
	@$(CC) $(SONGVIEW_OBJ) -o $(SONGVIEW_EXEC) $(SONGVIEW_LDFLAGS) || ( \
		echo "*** Building songview failed. It might be broken, or you might be missing libraries. ***"; \
		echo "Try running this command (ask an adult for help with 'sudo'):"; \
		echo "  sudo apt install build-essential libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev libfftw3-dev"; \
		echo "Then run 'make songview' again, but note songview will still not work."; \
		exit 1; \
	)
	@echo "songview built, but it will not work properly (it's a work in progress). Try './songview <filename>.song'."

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@ || ( \
		echo "*** Error: Failed to compile $<."; \
		echo "Make sure all source files are correct and try 'make clean' then 'make' again."; \
		echo "If it still fails, you might need libraries. Run (with adult help):"; \
		echo "  sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev"; \
		exit 1; \
	)

# Clean all built files
clean:
	@echo "Cleaning up old files..."
	@rm -rf $(OBJ_DIR) $(EXEC) $(SONGVIEW_EXEC) $(SONGGEN_EXEC) || ( \
		echo "*** Error: Could not clean files. Check if you have permission to delete files in this folder."; \
		echo "Ask an adult to run 'sudo rm -rf obj linesplus songgen songview' if needed."; \
		exit 1; \
	)
	@echo "*** Clean successful! All built files removed. Ready for a fresh build. ***"

.PHONY: all clean