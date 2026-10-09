cmake_minimum_required(VERSION 3.25)
project(itch-book CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)        # -std=c++20, а не -std=gnu++20

find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

add_library(itch STATIC src/itch/mapped_file.cpp) 
target_include_directories(itch PUBLIC include)
target_compile_options(itch PUBLIC -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_executable(tests
    tests/itch/test_endian.cpp
    tests/itch/test_mapped_file.cpp)
target_link_libraries(tests PRIVATE itch GTest::gtest_main)
add_test(NAME tests COMMAND tests)

add_executable(bench
    bench/itch/bench_endian.cpp
    bench/itch/bench_mapped_file.cpp)
target_link_libraries(bench PRIVATE itch benchmark::benchmark_main)