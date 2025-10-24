SHELL=sh
CXX = g++

TARGET_EXEC := mspice.exe

BUILD_DIR := ./build
SRC_DIR := ./src

SRCS := $(wildcard $(SRC_DIR)/*.cpp)

DEPS := $(wildcard $(SRC_DIR)/*.hpp)

# OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
OBJS := $(patsubst %.cpp,%.cpp.o,$(SRCS))
OBJS := $(patsubst $(SRC_DIR)%,$(BUILD_DIR)%,$(OBJS))

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -std=c++20 -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp $(DEPS)
	$(CXX) -c $< -std=c++20 -o $@

.PHONY: clean
clean:
	$(SHELL) -c "rm -r $(BUILD_DIR)/*"