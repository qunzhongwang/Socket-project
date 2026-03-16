CXX = g++
# CXXFLAGS = -Wall -std=c++11
CXXFLAGS = -w -std=c++11
BUILD_DIR = build


all: $(BUILD_DIR) $(BUILD_DIR)/robot $(BUILD_DIR)/student

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/robot: robot/robot.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/student: student/student.cc student/sockutil.cc student/sockutil.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ student/student.cc student/sockutil.cc

start-robot: $(BUILD_DIR)/robot
	$(BUILD_DIR)/robot

start-student: $(BUILD_DIR)/student
	$(BUILD_DIR)/student

clean:
	rm -f $(BUILD_DIR)/robot $(BUILD_DIR)/student

.PHONY: all clean start-robot start-student
