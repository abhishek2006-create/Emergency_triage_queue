# Emergency Triage Queue

This C++11 project compares an indexed binary min-heap, a lazy-invalidation binary heap, and an unsorted-array oracle. It includes queue checks, a patient triage simulator, and a benchmark.

## Requirements

- CMake 3.20 or newer
- A C++11 compiler
- SQLite3 development library

If you are using MSYS2 with the UCRT64 environment, SQLite3 can be installed with:

```powershell
pacman -S mingw-w64-ucrt-x86_64-sqlite3
```

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

The registered checks include the integration test, scale test, Oracle unit test, and database replay test.

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

## Run the database replay

The project includes a synthetic SQLite database containing 10,000 patient records and 20,000 queue events.

The replay program processes the events in chronological order and compares the results of the indexed heap, lazy heap, and Oracle.

Run from the project root:

```powershell
.\build\database_replay.exe .\data\emergency_triage_database_v2.db .\build\database_replay.csv
```

The program checks the queue operations and writes the replay results and timing measurements to the specified CSV file.

The database is synthetic benchmark data and does not contain real patient information. More details about the dataset and its generation are available in `data\README_database_v2.txt`.

## Run randomized differential testing

The project also includes a randomized differential test that compares Structure A, Structure B, and the Oracle using generated queue operations.

Run 100,000 operations with a fixed seed:

```powershell
.\build\oracle_differential.exe 100000 20261116
```

The arguments are the number of operations and the random seed. Using the same seed makes the test reproducible.

The test compares operation results, queue sizes, and invariants across all three implementations.
