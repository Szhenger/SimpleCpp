cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(traffic)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(traffic traffic.cpp)
target_link_libraries(traffic "${TORCH_LIBRARIES}" ${OpenCV_LIBS})
set_property(TARGET traffic PROPERTY CXX_STANDARD 23)
