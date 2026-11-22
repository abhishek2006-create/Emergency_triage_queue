# Tools and Assistance Disclosure

## Development environment

The project was developed using C++11, CMake, Git, and SQLite.

The final verified Windows build environment used:

- Operating system: Windows (PowerShell)
- Compiler: GCC 16.2.0 (MSYS2 UCRT64)
- Compiler executable: `C:\msys64\ucrt64\bin\g++.exe`
- CMake: 4.5.1
- Build generator: Ninja
- SQLite command-line tool: 3.54.1
- SQLite include directory: `C:/msys64/ucrt64/include`

The SQLite version above identifies the command-line tool; it is not asserted to be the exact linked library version.

## Assistance

ChatGPT was used as an assistance tool during project development for explanations, debugging support, code review, integration guidance, documentation review, and help with build and test verification.

The project implementation, design decisions, experimental interpretation, and final submission should be reviewed and confirmed by the team.

## Reproducibility

The simulator, benchmark, randomized differential test, and adversarial test support explicit seeds or fixed test parameters.

The README documents the commands used to configure, build, test, run simulations, execute benchmarks, and replay both database datasets.

Benchmark measurements depend on the workload and execution environment. They should be interpreted using the limitations described in the technical report.
