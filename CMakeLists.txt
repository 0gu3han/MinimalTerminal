# Minimum version raised from 3.10.0 to 3.28.0, which is required for C++ modules
cmake_minimum_required(VERSION 3.28.0)
project(dynodeck VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dynodeck src/main.cpp src/presentation/presentation.cpp src/presentation/slide.cpp src/presentation/styler.cpp src/presentation/terminal.cpp)
target_sources(dynodeck PUBLIC FILE_SET CXX_MODULES FILES src/presentation/presentation.cppm src/presentation/slide.cppm src/presentation/styler.cppm src/presentation/terminal.cppm)
