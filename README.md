

https://github.com/user-attachments/assets/5d730a41-f04b-4388-aa19-ca445910a9c0

# Emergency Triage Queue

A C++11 project comparing three priority-queue implementations for an emergency triage workload:

- **Structure A:** indexed binary min-heap with an open-addressing locator table.
- **Structure B:** binary min-heap with lazy invalidation and versioned entries.
- **Oracle:** unsorted-array reference implementation used for correctness comparisons.

The project includes queue tests, randomized differential testing, a patient triage simulator, database replay, adversarial tests, and an analyst benchmark.

## Team and contributions

## Team Name : CodeCraft

| Role | Team member | Roll number | Responsibility |
|---|---|---:|---|
| D1 – Structure A | Ankit | 250041003 | Indexed binary min-heap and locator |
| D2 – Structure B | Kethavath Mothilal | 250001039 | Lazy-invalidation binary heap |
| D3 – Architect | Abhishek Bairwa | 250001002 | Shared interface and integration |
| D4 – Oracle | Abhimanyu Kumar | 250002005 | Reference implementation and correctness checking |
| D5 – Adversary | Sunil Mudavath | 250001049 | Adversarial workloads and stress testing |
| D6 – Analyst | Harshwardhan Mukhedkar | 250041017 | Cost analysis and performance measurements |

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
cmake --build build -j 4
```

## Run the complete build and test suite

The repository includes a Bash helper that configures the project, builds it, and runs all registered CTest checks:

```bash
bash scripts/run_all.sh
```

Run this command from Git Bash or another Bash environment on Windows. Alternatively, use PowerShell:

```powershell
cmake -S . -B build
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

The registered checks are:

1. integration
2. scale
3. oracle_unit
4. database_replay
5. adversary
6. million_differential
7. uploaded_dataset_replay

## Run the integration check

```powershell
.\build\test_integration.exe
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

The benchmark varies the revision share in the generated operation workload and records timing and memory-related measurements. See `docs/derivations/D6_Analyst.md` and the technical report for the cost model, measurement method, results, and limitations.

## Run the database replay

The project includes a synthetic SQLite database containing 10,000 patient records and 20,000 queue events.

The replay program processes the events in chronological order and compares the results of the indexed heap, lazy heap, and Oracle.

Run from the project root:

```powershell
.\build\database_replay.exe .\data\emergency_triage_database_v2.db .\build\database_replay.csv
.\build\database_replay.exe .\data\emergency_triage_synthetic.db .\build\uploaded_dataset_replay.csv
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

The test compares operation results, queue sizes, and invariants across all three implementations. The registered `million_differential` CTest performs the larger differential test.

## Adversarial testing

The registered `adversary` test exercises stress scenarios including heap sift operations, shuffled extraction, repeated revisions, departures, and starvation-policy cases. Raw adversarial counters are stored in `results/raw/adversary_results.csv`; the recorded figures are included in `results/figures/`.

To run the adversarial program manually from PowerShell:

```powershell
New-Item -ItemType Directory -Force .\results\raw | Out-Null
.\build\run_adversary.exe 100000 20261116 .\results\raw\adversary_manual.csv
```

The arguments are workload size, random seed, and output CSV path, respectively.

## Results and documentation

- [Technical report](REPORT.pdf) — full project explanation, analysis, experiments, results, limitations, and figures.
- [Technical report source](REPORT.md)
- [Shared interface and queue contract](INTERFACE.md)
- [Tools and assistance disclosure](TOOLS.md)
- [Figure captions and data notes](results/figures/FIGURE_CAPTIONS.md)

### Derivation documents

The six role derivations are stored in `docs/derivations/`:

- `D1_Structure_A.md` and `D1_Structure_A.pdf` — indexed heap complexity proof.
- `D2_Structure_B.pdf` — lazy-heap analysis.
- `D3_Architect.pdf` — architecture and integration derivation.
- `D4_Oracle.md` and `D4_Oracle.pdf` — locator invariant and correctness proof.
- `D5_Adversary.pdf` — adversarial analysis.
- `D6_Analyst.md` and `D6_Analyst.pdf` — cost model and measurement analysis.

## Reproducibility and interpretation

Use fixed seeds to reproduce generated workloads. Keep the compiler, build configuration, operation count, and seed consistent when comparing runs.

Benchmark measurements are workload- and environment-specific. The current analyst benchmark varies revision share; it does not independently compare the runtime cost of the linear-aging and stepwise-aging simulator policies. Process-level peak-memory measurements should not be interpreted as isolated per-structure memory usage.

The simulator uses synthetic arrivals and service times. Its results are algorithmic experiment outputs, not clinical evidence.
