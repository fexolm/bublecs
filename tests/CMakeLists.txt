include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(ecs_test ecs_test.cpp)
target_link_libraries(ecs_test bublecs GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ecs_test)