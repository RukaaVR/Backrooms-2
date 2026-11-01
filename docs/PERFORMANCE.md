# Performance

Rule: only measured numbers are recorded. Nothing here is estimated or extrapolated.

## Target
* 1920×1080 at 60 FPS (16.6 ms) on a documented midrange PC.
* Reference midrange PC (target, not yet tested): 6-core desktop CPU (e.g. Ryzen 5 3600 / i5-10400),
  16 GB RAM, GTX 1660 Super / RX 5600 XT class GPU (~5 TFLOPS), SSD, Windows 10/11 or Linux.
* Stable frame pacing: p99 frame time within 1.5× of the average; no streaming hitch > 33 ms.

## How to measure
```bash
build/bin/BackroomsBeyondReality --level level0 --seed 1337 --width 1920 --height 1080 --no-vsync --benchmark --frames 3000
```
`--benchmark` logs frame count, average, p50, p99 and max frame time on exit. The built-in
profiler (`engine/core/Profiler.h`, `BR_PROFILE_SCOPE`) records per-scope CPU time; configure
with `-DBR_ENABLE_TRACY=ON` for live Tracy capture. GPU pass timings come from
`render::FrameStats`.

## Measurements

| Date | Hardware | Build | Scene | Resolution / settings | Avg | p99 | Notes |
|---|---|---|---|---|---|---|---|
| — | — | — | — | — | — | — | No real-GPU measurement yet. The development container only has Mesa llvmpipe (CPU software rasteriser); its timings are not representative and are deliberately not recorded as performance results. |

## Budgets (design, to be validated on hardware)
| Area | Budget @1080p |
|---|---|
| G-buffer + shadows (cached static) | 4.0 ms GPU |
| Lighting (clustered) + GI approximation | 3.5 ms GPU |
| AO + SSR + volumetrics | 3.0 ms GPU |
| Post (TAA, bloom, exposure, tonemap) | 1.5 ms GPU |
| UI | 0.3 ms GPU |
| Main thread simulation (physics, AI, gameplay, audio) | 5.0 ms CPU |
| Streaming activation (per frame) | ≤ 2.0 ms CPU |
