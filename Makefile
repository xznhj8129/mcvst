# Makefile
CXX := g++
TARGET := tracker

SRC_DIR := src
OBJ_DIR := obj

CPPFLAGS := -Iinclude -MMD -MP $(shell pkg-config opencv4 gstreamer-1.0 gstreamer-app-1.0 --cflags) 
CXXFLAGS := -Wfatal-errors -fmax-errors=1  -std=c++17 -Wall -Wextra -Ofast -march=native -mcpu=native -mtune=native
LDFLAGS  := $(shell pkg-config opencv4 gstreamer-1.0 gstreamer-app-1.0 --libs-only-L)
LDLIBS   := $(shell pkg-config opencv4 gstreamer-1.0 gstreamer-app-1.0 --libs-only-l) -pthread -lconfig++ -lcurl -pg

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

.PHONY: all clean

.PHONY: parallel

parallel:
	$(MAKE) -j$(NPROC)
