add_executable(navigation_test navigation_test.cpp)

target_include_directories(navigation_test PRIVATE
  ${PROJECT_SOURCE_DIR}
)

add_test(NAME navigation_test COMMAND navigation_test)