cmake_minimum_required(VERSION 3.20)
project(emergency_triage_queue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(triage_queues
    src/structA/indexed_heap.cpp
    src/structA/locator_table.cpp
    src/structB/lazy_heap.cpp
    oracle/triage_oracle.cpp
)
target_include_directories(triage_queues PUBLIC src)

add_executable(test_integration tests/test_integration.cpp)
target_link_libraries(test_integration PRIVATE triage_queues)

add_executable(test_million tests/test_million.cpp)
target_link_libraries(test_million PRIVATE triage_queues)

add_executable(test_scale tests/test_scale.cpp)
target_link_libraries(test_scale PRIVATE triage_queues)

add_executable(run_simulator simulator/run_simulator.cpp)
target_link_libraries(run_simulator PRIVATE triage_queues)

add_executable(analyst_benchmark analyst/benchmark.cpp)
target_link_libraries(analyst_benchmark PRIVATE triage_queues)

enable_testing()
add_test(NAME integration COMMAND test_integration)
add_test(NAME scale COMMAND test_scale)
