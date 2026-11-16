$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$log = Join-Path $root 'results\verification.txt'
New-Item -ItemType Directory -Force -Path (Join-Path $root 'results') | Out-Null
"Verification started: $(Get-Date -Format o)" | Set-Content -LiteralPath $log
function Run-Build {
    param([string]$label, [string[]]$compileArgs)
    "`n[$label] g++ $($compileArgs -join ' ')" | Add-Content -LiteralPath $log
    & g++ @compileArgs 2>&1 | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw "$label compilation failed" }
}
$common = @('src/structA/indexed_heap.cpp', 'src/structA/locator_table.cpp', 'src/structB/lazy_heap.cpp')
Run-Build 'integration' (@('-std=c++11', '-O2', '-Wall', '-Wextra', '-pedantic', 'tests/test_integration.cpp') + $common + @('-o', 'results/test_integration.exe'))
& results/test_integration.exe 2>&1 | Tee-Object -FilePath $log -Append
if ($LASTEXITCODE -ne 0) { throw 'Integration checks failed' }
Run-Build 'million differential' (@('-std=c++11', '-O2', '-Wall', '-Wextra', '-pedantic', 'oracle/differential_test.cpp', 'oracle/triage_oracle.cpp') + $common + @('-o', 'results/differential.exe'))
& results/differential.exe 1000000 20261116 2>&1 | Tee-Object -FilePath $log -Append
if ($LASTEXITCODE -ne 0) { throw 'Million-operation differential test failed' }
Run-Build 'full-scale construction/extraction' (@('-std=c++11', '-O2', '-Wall', '-Wextra', '-pedantic', 'tests/test_scale.cpp') + $common + @('-o', 'results/test_scale.exe'))
& results/test_scale.exe 2>&1 | Tee-Object -FilePath $log -Append
if ($LASTEXITCODE -ne 0) { throw 'Full-scale test failed' }
Run-Build 'adversarial million' (@('-std=c++11', '-O2', '-Wall', '-Wextra', '-pedantic', 'adversary/adversary.cpp', 'adversary/run_adversary.cpp', 'oracle/triage_oracle.cpp') + $common + @('-o', 'results/adversary.exe'))
& results/adversary.exe 1000000 20261116 results/adversary_1m.csv 2>&1 | Tee-Object -FilePath $log -Append
if ($LASTEXITCODE -ne 0) { throw 'Adversarial run failed' }
try {
    Run-Build 'sanitizer integration' (@('-std=c++11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', 'tests/test_integration.cpp') + $common + @('-o', 'results/test_sanitizer.exe'))
    & results/test_sanitizer.exe 2>&1 | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw 'Sanitizer integration failed' }
}
catch {
    "Sanitizer unavailable or failed: $_" | Tee-Object -FilePath $log -Append
}
Run-Build 'simulator' (@('-std=c++11', '-O2', '-Wall', '-Wextra', '-pedantic', 'simulator/run_simulator.cpp') + $common + @('-o', 'results/simulator.exe'))
& results/simulator.exe 20261116 results/simulator.csv 2>&1 | Tee-Object -FilePath $log -Append
if ($LASTEXITCODE -ne 0) { throw 'Simulator failed' }
Run-Build 'benchmark' (@('-std=c++11', '-O2', '-Wall', '-Wextra', '-pedantic', 'analyst/benchmark.cpp', 'oracle/triage_oracle.cpp') + $common + @('-o', 'results/analyst_benchmark.exe', '-lpsapi'))
& results/analyst_benchmark.exe 20000 20261116 results/analyst.csv 2>&1 | Tee-Object -FilePath $log -Append
if ($LASTEXITCODE -ne 0) { throw 'Benchmark failed' }
"Verification finished: $(Get-Date -Format o)" | Add-Content -LiteralPath $log
