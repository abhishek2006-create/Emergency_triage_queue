# Emergency Triage Queue

This C++11 project compares an indexed binary min-heap, a lazy-invalidation binary heap, and an unsorted-array oracle. It includes queue checks, a patient triage simulator, and a benchmark.

## Requirements

- CMake 3.20 or newer
- A C++11 compiler

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

Run from the project root so the CSV is written under `results`:

```powershell
.\build\run_simulator.exe 20261116 results\simulator.csv 30
```

The arguments are seed, output CSV path, and simulation duration in days. Change `30` to any positive number of days. The simulator prints the requested duration and summary statistics, then writes per-severity wait statistics to the CSV. It generates arrivals throughout the requested horizon.

## Run the benchmark

```powershell
.\build\analyst_benchmark.exe 20000 20261116 results\analyst.csv
```
