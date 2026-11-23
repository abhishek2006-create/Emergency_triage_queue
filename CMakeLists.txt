cmake_minimum_required(VERSION 3.20)
project(emergency_triage_queue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(SQLite3 REQUIRED)

add_library(triage_queues
    src/structA/indexed_heap.cpp
    src/structA/locator_table.cpp
    src/structB/lazy_heap.cpp
    oracle/triage_oracle_impl.cpp
)
target_include_directories(triage_queues PUBLIC src)

add_executable(test_oracle oracle/triage_oracle.cpp)
target_link_libraries(test_oracle PRIVATE triage_queues)

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

add_executable(oracle_differential "oracle/test_oracle .cpp")
target_link_libraries(oracle_differential PRIVATE triage_queues)

add_executable(database_replay database/database_replay.cpp)
target_link_libraries(database_replay PRIVATE triage_queues SQLite3::SQLite3)

add_executable(run_adversary
    adversary/run_adversary.cpp
    adversary/adversary.cpp
)

target_link_libraries(run_adversary PRIVATE triage_queues)

enable_testing()
add_test(NAME integration COMMAND test_integration)
add_test(NAME scale COMMAND test_scale)
add_test(NAME oracle_unit COMMAND test_oracle)

add_test(
    NAME database_replay
    COMMAND database_replay
        "${CMAKE_SOURCE_DIR}/data/emergency_triage_database_v2.db"
        "${CMAKE_BINARY_DIR}/database_replay.csv"
)

add_test(
    NAME adversary
    COMMAND run_adversary
        100000
        20261116
        "${CMAKE_BINARY_DIR}/adversary_results.csv"
)

add_test(
    NAME million_differential
    COMMAND test_million
)

add_test(
    NAME uploaded_dataset_replay
    COMMAND database_replay
        "${CMAKE_SOURCE_DIR}/data/emergency_triage_synthetic.db"
        "${CMAKE_BINARY_DIR}/database_replay_uploaded_dataset.csv"
)
