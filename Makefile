# Makefile
CXX := g++
TARGET := tracker

SRC_DIR := src
OBJ_DIR := obj

CPPFLAGS := -Iinclude -MMD -MP `pkg-config opencv4 --cflags`
CXXFLAGS := -std=c++17 -Wall -Wextra -Ofast -march=native -mcpu=native -mtune=native -pg
LDFLAGS  := $(shell pkg-config --libs-only-L opencv4)
LDLIBS   := $(shell pkg-config --libs-only-l opencv4) -pthread -lconfig++ -pg

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPENDS := $(OBJECTS:.o=.d)
NPROC := $(shell nproc)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

-include $(DEPENDS)

clean:
	rm -rf $(OBJ_DIR) 
	rm $(TARGET)

.PHONY: all clean

.PHONY: parallel
parallel:
	$(MAKE) -j$(NPROC)