CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
SRC      = src/main.cpp src/SegmentTree.cpp src/GPUDevice.cpp \
           src/JobManager.cpp src/Scheduler.cpp src/CLI.cpp src/Visualizer.cpp
TARGET   = gpualloc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
