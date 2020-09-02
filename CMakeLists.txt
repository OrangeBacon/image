cmake_minimum_required (VERSION 3.8)

project(image LANGUAGES CXX)

add_compile_options(/utf-8 /std:c++latest)
add_executable(image
	src/main.cpp
	src/png.cpp
	src/png.hpp
)
