cmake_minimum_required(VERSION 3.28.0)
project(dynodeck VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CURSES_NEED_NCURSES TRUE)
# the wide version draws … and other non-ASCII text. Linux has it as its own
# library, and the ncurses that comes with macOS already is one.
find_library(NCURSESW_LIBRARY ncursesw)
if(NCURSESW_LIBRARY)
  set(CURSES_NEED_WIDE TRUE)
endif()
find_package(Curses REQUIRED)

# layout has no curses in it, so the tests can use it on its own
add_library(layout src/presentation/layout.cpp)
target_sources(layout PUBLIC FILE_SET CXX_MODULES FILES src/presentation/layout.cppm)

# everything but main, so the tests can use it too
add_library(deck
  src/presentation/presentation.cpp
  src/presentation/slide.cpp
  src/presentation/editor.cpp
  src/presentation/styler.cpp
  src/presentation/highlighter.cpp)
target_sources(deck PUBLIC FILE_SET CXX_MODULES FILES
  src/presentation/presentation.cppm
  src/presentation/slide.cppm
  src/presentation/editor.cppm
  src/presentation/styler.cppm
  src/presentation/highlighter.cppm)
target_include_directories(deck PUBLIC ${CURSES_INCLUDE_DIRS})
target_link_libraries(deck PUBLIC layout ${CURSES_LIBRARIES})

add_executable(dynodeck src/main.cpp)
target_link_libraries(dynodeck PRIVATE deck)

enable_testing()
add_library(testing)
target_sources(testing PUBLIC FILE_SET CXX_MODULES FILES tests/testing.cppm)

add_executable(layout_test tests/layout_test.cpp)
target_link_libraries(layout_test PRIVATE layout testing)
add_test(NAME layout COMMAND layout_test)

add_executable(editor_test tests/editor_test.cpp)
target_link_libraries(editor_test PRIVATE deck testing)
add_test(NAME editor COMMAND editor_test)

add_executable(slide_test tests/slide_test.cpp)
target_link_libraries(slide_test PRIVATE deck testing)
add_test(NAME slide COMMAND slide_test)

add_executable(styler_test tests/styler_test.cpp)
target_link_libraries(styler_test PRIVATE deck testing)
add_test(NAME styler COMMAND styler_test)
