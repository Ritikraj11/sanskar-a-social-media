cmake_minimum_required(VERSION 3.14)

# Project name and version
project(SanskarApp VERSION 1.0)

# Enable Conan's CMake integration
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add your source files here
add_executable(SanskarApp main.cpp)

# Link the dependencies from Conan
target_link_libraries(SanskarApp ${CONAN_LIBS})
