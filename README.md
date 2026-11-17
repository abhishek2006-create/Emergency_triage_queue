# Emergency Triage Queue

This C++11 project compares an indexed binary min-heap, a lazy-invalidation binary heap, and an unsorted-array oracle. It also includes queue checks, a patient triage simulator, and a benchmark.

## Requirements

- CMake 3.20 or newer
- A C++11 compiler (MinGW g++ works on Windows)

## Build

From the project root in PowerShell:

```powershell
cmake -S . -B build
cmake --build build
```

## Run the integration check

```powershell
.\build\test_integration.exe
```

Or run registered checks with:

```powershell
ctest --test-dir build --output-on-failure
```

## Run the triage simulation

Run this from the project root so the CSV is written under `results`:

```powershell
.\build\run_simulator.exe 20261116 results\simulator.csv
```

The simulator compares no-aging, linear-aging, and stepwise-aging policies over a 30-day simulated horizon. It prints summary statistics and writes per-severity wait statistics to the CSV.

To run the benchmark:

```powershell
.\build\analyst_benchmark.exe 20000 20261116 results\analyst.csv
```

<!-- 
g++ -std=c++11 -O2 -Wall -Wextra -pedantic -Isrc simulator/run_simulator.cpp src/structA/indexed_heap.cpp src/structA/locator_table.cpp src/structB/lazy_heap.cpp -o results\simulator.exe

.\results\simulator.exe 20261116 results\simulator.csv -->
