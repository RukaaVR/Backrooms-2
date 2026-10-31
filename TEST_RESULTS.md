# Test Results

Only results that were actually executed are recorded here.

## 2026-10-31 — wave 0 foundation
Environment: Ubuntu 24.04, GCC 13.3, Release, Mesa llvmpipe (software GL 4.5) under Xvfb.

| Suite | Result |
|---|---|
| Build (`cmake --build build`) | OK, 0 project warnings |
| `ctest` | 11/11 passed (core RNG/hash/noise/slotmap/jobs/fs, settings, input bindings) |
| Smoke run `--screenshot` (960×540, 10 warm-up frames) | OK, image inspected |

Performance: not measured (software rasteriser; not representative).
