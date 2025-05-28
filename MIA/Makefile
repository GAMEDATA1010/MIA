# Makefile for the Gemini C++ Application

# --- Configuration Variables ---
# C++ compiler to use
CXX = g++

# C++ standard
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

# Directories for headers and libraries
# Adjust these paths based on your system's curl installation.
# The path below is derived from your `find` command output for Linuxbrew.
CURL_INCLUDE_DIR = /var/home/linuxbrew/.linuxbrew/Cellar/curl/8.13.0/include
CURL_LIB_DIR = /var/home/linuxbrew/.linuxbrew/Cellar/curl/8.13.0/lib

# Include paths (-I for headers)
# -I. for current directory (where json.hpp and your .h files are)
# -I$(CURL_INCLUDE_DIR) for curl headers
INCLUDE_PATHS = -I. -I$(CURL_INCLUDE_DIR)

# Library paths (-L for linker to find libraries)
# -L$(CURL_LIB_DIR) for curl library
LIB_PATHS = -L$(CURL_LIB_DIR)

# Libraries to link against (-l for specific libraries)
# -lcurl for libcurl
# -lstdc++fs for filesystem (may not be needed on newer g++ versions)
LIBS = -lcurl -lstdc++fs

# Output executable name
TARGET = Mia_App

# --- Source File Discovery ---
# Find all .cpp files in the current directory
# This is the key to future-proofing: just add new .cpp files, and make will find them.
SRCS = $(wildcard *.cpp)

# Generate object file names from source file names
# e.g., main.cpp -> main.o, agent.cpp -> agent.o
OBJS = $(SRCS:.cpp=.o)

# --- Makefile Rules ---

# Default target: builds the executable
all: $(TARGET)

# Rule to build the executable from object files
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJS) $(LIB_PATHS) $(LIBS) -o $(TARGET)
	@echo "Compilation successful! Executable: ./$(TARGET)"

# Rule to compile each .cpp file into an .o object file
# $< is the first prerequisite (the .cpp file)
# $@ is the target (the .o file)
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATHS) -c $< -o $@

# Clean rule: removes object files and the executable
clean:
	@echo "Cleaning up..."
	@rm -f $(OBJS) $(TARGET)
	@echo "Clean complete."

# Phony targets: prevent conflicts with files of the same name
.PHONY: all clean

