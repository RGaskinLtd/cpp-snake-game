SRC_DIR = src
BUILD_DIR = build/debug
CC = clang++
SRC_FILES = $(wildcard $(SRC_DIR)/test.cpp)
OBJ_NAME = game
COMPILER_FLAGS = -std=c++11 -Wall -O0 -g -arch x86_64 # or arm64
LINKER_FLAGS = -framework OpenGL -lGL -lglut -lGLESv2
all:
	$(CC) $(COMPILER_FLAGS) $(LINKER_FLAGS) $(SRC_FILES) -o $(BUILD_DIR)/$(OBJ_NAME)
clean:
	rm -r -f $(BUILD_DIR)/*
.PHONY: all clean
