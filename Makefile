# Makefile  ── full, self-contained
CXX      := g++
TARGET   := tracker

# Source / object layout
SRC_DIR  := src
OBJ_DIR  := obj

# Which build?  release (default) or debug
BUILD    ?= release

# ---------------------------------------------------------------------------
# External libraries via pkg-config
# ---------------------------------------------------------------------------
PKG_CFLAGS := $(shell pkg-config opencv4 gstreamer-1.0 gstreamer-app-1.0 --cflags)
PKG_LIB_L  := $(shell pkg-config opencv4 gstreamer-1.0 gstreamer-app-1.0 --libs-only-L)
PKG_LIB_l  := $(shell pkg-config opencv4 gstreamer-1.0 gstreamer-app-1.0 --libs-only-l)

# ---------------------------------------------------------------------------
# Common flags
# ---------------------------------------------------------------------------
CPPFLAGS  := -Iinclude -MMD -MP $(PKG_CFLAGS)
BASE_CXX  := -std=c++17 -Wall -Wextra -Wfatal-errors -fmax-errors=1 -pthread
LDFLAGS   := $(PKG_LIB_L)
BASE_LIBS := $(PKG_LIB_l) -pthread -lconfig++ -lcurl

# ---------------------------------------------------------------------------
# Build-type-specific flags
# ---------------------------------------------------------------------------
ifeq ($(BUILD),debug)
    # Symbols + frame pointers for perf/valgrind/gdb
    CXXFLAGS := $(BASE_CXX) -O2 -g -fno-omit-frame-pointer
    LDLIBS   := $(BASE_LIBS)
else
    # Old “fast” flags
    CXXFLAGS := $(BASE_CXX) -Ofast -march=native -mcpu=native -mtune=native
    LDLIBS   := $(BASE_LIBS)
endif

# ---------------------------------------------------------------------------
# File lists
# ---------------------------------------------------------------------------
SOURCES  := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPENDS  := $(OBJECTS:.o=.d)
NPROC    := $(shell nproc)

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

-include $(DEPENDS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

parallel:
	$(MAKE) -j$(NPROC) BUILD=$(BUILD)

.PHONY: all clean parallel
