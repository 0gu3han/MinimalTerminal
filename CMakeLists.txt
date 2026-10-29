cmake_minimum_required(VERSION 3.28.0)
project(dynodeck VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(dynodeck src/main.cpp src/presentation/presentation.cpp src/presentation/slide.cpp src/presentation/styler.cpp)
target_sources(dynodeck PUBLIC FILE_SET CXX_MODULES FILES src/presentation/presentation.cppm src/presentation/slide.cppm src/presentation/styler.cppm)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
target_include_directories(dynodeck PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(dynodeck PRIVATE ${CURSES_LIBRARIES})
