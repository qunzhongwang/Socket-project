CXX = g++
# CXXFLAGS = -Wall -std=c++11
CXXFLAGS = -w -std=c++11

all: robot student

robot: robot/robot.cc
	$(CXX) $(CXXFLAGS) -o build/$@ $<

student: student/student.cc
	$(CXX) $(CXXFLAGS) -o build/$@ $<

clean:
	rm -f robot/robot student/student

.PHONY: all clean