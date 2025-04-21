CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = -lncurses
PKG_CONFIG = `pkg-config --cflags --libs libnotify 2>/dev/null || echo ""`

# If pkg-config found libnotify, add it to flags
ifneq ($(PKG_CONFIG),)
    CXXFLAGS += $(PKG_CONFIG) -DHAS_LIBNOTIFY
endif

TARGET = activity_monitor
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.d,$(SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS) 