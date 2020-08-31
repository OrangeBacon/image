cmake_minimum_required (VERSION 3.8)

project(image)

add_executable(image
	src/main.cpp
	src/png.cpp
	src/png.hpp
)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
