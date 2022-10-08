cmake_minimum_required(VERSION 3.14)

project(bublic)

enable_testing()
add_subdirectory(bublecs)
add_subdirectory(tests)

add_executable(main main.cpp)
target_link_libraries(main bublecs)