# Live Demonstration: Fresh Clone at Full Scale

The presentation requirement says the demonstration must start from a fresh clone of the release tagged `final`. This project folder currently has no `.git` directory or remote, so it is not yet possible to demonstrate that condition from this folder alone. Put this folder in the team's GitHub repository, push the final commit, and push a Git tag named `final` before the presentation.

## Prepare the release once

From the project folder, after creating an empty GitHub repository and copying its URL:

```powershell
git init
git add .
git commit -m "Emergency Triage Queue final"
git branch -M main
git remote add origin <repository-url>
git push -u origin main
git tag -a final -m "Final release"
git push origin final
```

If the team already has a Git repository, do not run `git init`; commit the project into that repository and create/push the `final` tag there. Confirm the tag appears in the repository's Releases/Tags page. A GitHub Release can be created from the pushed `final` tag if the instructor expects the GitHub Releases page specifically.

## Rehearse the fresh-clone demo

Open PowerShell and replace `<repository-url>` with the repository's clone URL:

```powershell
git clone --branch final --depth 1 <repository-url> EmergencyTriageQueue-demo
cd EmergencyTriageQueue-demo
git describe --tags --exact-match
powershell -ExecutionPolicy Bypass -File scripts\verify.ps1
```

The tag command should print `final`. The script compiles and runs the integration and tie-breaking checks, one-million-operation A/B/Oracle differential test, one-million-element build/extraction test, one-million-scale adversarial workloads, sanitizer attempt, 30-day simulator, and benchmark. The current MinGW GCC 6.3.0 has an internal compiler error in its `<random>` header during sanitizer compilation; the script records that compiler limitation and continues. If your presentation computer has a different sanitizer-capable compiler, rebuild the sanitizer target there.

## Suggested 15-minute presentation use

1. Start from the clone and show `git describe --tags --exact-match` returning `final`.
2. Run the verification script. Keep `results\verification.txt` open to show the full-scale passes and the sanitizer compiler limitation.
3. Show `results\adversary_1m.csv` for the measured million-scale workload counters.
4. Show `results\simulator.csv` for the three aging policies and per-severity waiting summaries. Explain that the one patient still waiting at the 30-day horizon is excluded from treated-patient wait statistics.
5. Show `results\analyst.csv` and explain that the five repetitions are a small smoke benchmark; the measured 20,000-operation runs are too short for a strong speed comparison.

Before presenting, confirm the repository URL and pushed tag with the instructor's expected access. The demo never needs patient data or network access after cloning; compilation and tests run locally.
