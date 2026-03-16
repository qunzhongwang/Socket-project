cmake_minimum_required(VERSION 3.10)
project(IERG3310_Socket LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_executable(robot robot/robot.cc)
add_executable(student student/student.cc student/sockutil.cc)