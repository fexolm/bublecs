cmake_minimum_required(VERSION 3.14)

project(bublic)

enable_testing()
add_subdirectory(bublecs)
add_subdirectory(tests)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} bublecs)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 LINKER_LANGUAGE CXX)
