cmake_minimum_required(VERSION 3.20)
project(emergency_triage_queue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(triage_contract INTERFACE)
target_include_directories(triage_contract INTERFACE include)

add_executable(contract_example tests/contract_example.cpp)
target_link_libraries(contract_example PRIVATE triage_contract)

enable_testing()
add_test(NAME contract_example COMMAND contract_example)
