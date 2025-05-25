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
# libgl1-mesa-dev - OpenGL to draw graphics to the screen (songgen does not need)
# libpthread-stubs0-dev - Lets code create more threads to distribute workload. 8 core cpu uses 16, 16-32 etc.
# 1 core, 4 core exist no longer from 20 25.
#
# TROUBLESHOOTING:
# - If the build fails, it might mean a library is missing. The error messages below will tell you what to install.
# - If you see a password prompt with 'sudo', ask an adult to help.
# - If you get stuck, ask an adult or a friend who knows coding!
#
# MODIFYING:
# - songgen is songgen.cpp songgen.h instruments.h
# - linesplus is everything else. This means audio.cpp and audio.h too.
# - songview is songview.cpp songview.h - garbage, you can delete, cya next update?

# Compiler and flags (these tell the computer how to build the programs)
CC = g++
CFLAGS = -Wall -O3 -Iinclude -std=c++17
LDFLAGS = -lSDL2 -lSDL2_image -lGL -pthread
SONGGEN_LDFLAGS = -lSDL2 -pthread

# I will update if songview is ever not garbage. I like it that way for now.
# waste of my time, your time. .song is an only useful 'here' to play with tone generators.
# why view it? When it is better spent improving it.
# make - it sound better
# We have strict limitations because of having a tone generator
# How good can you get AI do with the code and an instrument you would like to hear?
# We generate tones within the range of human hearing. Do not be mean to pets.
# I have bigger plans for many of the instruments. generateSyntharp is probably too strong.
# I like the Plane9 windowed program, it dances with music.
# I listen to music with it on sometimes. Works with windows, This currently does not.
# I had it add some WIN32 on the audio files. Too lazy for the rest with other activities and my AI keeps breaking it.
# I use wine to run plane9. I use Port Proton to manage wine.
# www.plane9.com
# they have tools to make your own Plane9 'visualizers'. Shows audio and music with rainbows and math.
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
SONGGEN_EXEC = songgen

# If you have time then work on the better stuff ^
# instruments.h and songgen.h, in order of difficulty and rewards for better instruments, to get started.
SONGVIEW_EXEC = songview

# Default target: build ./linesplus and ./songgen
all: $(EXEC) $(SONGGEN_EXEC)
	@echo "*** Successfully built linesplus and songgen! ***"
	@echo "* Run './linesplus' to play or './songgen' to create a song. ***"
	@echo "** When playing ***"
	@echo "* Press F to exit fullscreen in ./linesplus. ***"
	@echo "* Press M to mute ./songgen from ./linesplus. ***"
	@echo "* Press ESC to exit ./linesplus. ***"
	@echo "* Press CTRL-C to exit ./songgen. ***"

# Build ./linesplus
$(EXEC): $(OBJECTS)
	@echo "Building linesplus..."
	@$(CC) $(OBJECTS) -o $(EXEC) $(LDFLAGS) || ( \
		echo "*** Failed! Building linesplus failed. You might be missing some libraries. ***"; \
		echo "*** Try running this command (ask an adult for help with 'sudo'):"; \
		echo "sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev"; \
		echo "* Then run 'make' again."; \
		exit 1; \
	)
	@echo "*** linesplus built successfully! Run './linesplus' to play. ***"
	@echo "* linesplus built successfully!"
	
# Build ./songgen
$(SONGGEN_EXEC): $(SONGGEN_OBJ)
	@echo "Building songgen..."
	@$(CC) $(SONGGEN_OBJ) -o $(SONGGEN_EXEC) $(SONGGEN_LDFLAGS) || ( \
		echo "*** Building songgen failed. You might be missing some libraries. ***"; \
		echo "Try running this command (ask an adult for help with 'sudo'):"; \
		echo "** sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev"; \
		echo "*Then run 'make songgen' again."; \
		exit 1; \
	)
	@echo "** songgen built successfully! Run './songgen' to create a song. ***"

# Build songview executable (only when explicitly requested)
$(SONGVIEW_EXEC): $(SONGVIEW_OBJ)
	@echo "Building songview (warning: it's broken and will not work. kek)..."
	@$(CC) $(SONGVIEW_OBJ) -o $(SONGVIEW_EXEC) $(SONGVIEW_LDFLAGS) || ( \
		echo "*** Building songview failed. It might be broken, or you might be missing libraries. ***"; \
		echo "Try running this command (ask an adult for help with 'sudo'):"; \
		echo "*  sudo apt install build-essential libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev libfftw3-dev"; \
		echo "Then run 'make songview' again, but note songview will still not work."; \
		exit 1; \
	)
	@echo "songview built, but it will not work properly (1 through crash). Try './songview song1.song'."

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@ || ( \
		echo "*** Error: Failed to compile $<."; \
		echo "Make sure all source files are correct and try 'make clean' then 'make' again."; \
		echo "If it still fails, you might need program libraries."; \
		echo "*  sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev"; \
		exit 1; \
	)

# If you have working linesplus and songgen executables you want to keep, copy them somewhere first
# Clean all built files. The command is rm for remove, it removes and calls it cleaning. Not a match.
# I run clean before uploading the files you have now. You can download those anytime.
# The only files it removes are the files that make creates.
# the objects and stuff, see below.
# Those are all machine readable files. Editing my files are safe changes.
# Always run 'make clean' after saving your changes.
# If you have working linesplus and songgen executables you want to keep, copy them somewhere first
# So you do not have to download again. Ad Free. github.com/ZacGeurts
# If you got it from somewhere else, it is not from the original orange.
clean:
	@echo "*** removing machine files."
	@rm -rf $(SONGVIEW_EXEC) $(OBJ_DIR) $(EXEC) $(SONGGEN_EXEC) || ( \
		echo "*** ::: Could not remove files. Oh well..."; \
		echo "** you better not be hiding in there songview."; \
		echo "* New builds will not build or be updated."; \
		echo "we do not have rm permissions."; \
		echo "we do not have remove permishuns."; \
		echo "this is an error you should not be seeing"; \
		echo "do not sudo this file if it has been altered, fix your code."; \
		echo "*** Do not alter this Makefile"; \
		echo "*** it is already designed to search for any include folder .h and src folder .cpp"; \
		exit 1; \
	)
	@echo "*** removing machine files."
	@echo "** Ready for a fresh build. Clean successful! All pre-built files removed.  ***"
	@echo "* type 'make' or 'make -j$(nproc)' you are ready for the not boring ***"

# nproc uses your full processor cores during the building process. You can specify -j16 or whatever.
# nproc just tells you the number. $(programname)'sudo apt install nproc'
# make - makes the programs (uses the build-essentials).
# if you change it, (ships pre-cleaned) github.com/ZacGeurts
# make clean
# make
# If you have working linesplus and songgen executables you want to keep, copy them somewhere first
# if you break it, delete it and download again, Ad Free. ./linesplus ./songgen
# songview.cpp and songview.h are broken code and maybe will work next update.
.PHONY: all clean