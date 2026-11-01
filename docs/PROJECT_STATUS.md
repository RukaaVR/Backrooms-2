# Project Status

_Last updated: 2026-11-01 (wave 0 complete; V4 brief adopted; wave 1 in progress)_

## Current phase
PHASE 1 complete (foundation) → PHASES 2–9 running in parallel as wave 1
(renderer, lighting/materials, physics/player, Level 0 generation, audio, UI/save).

## Brief
The project follows MASTER PROMPT V4 (fidelity over quantity, authored canonical layouts where
references exist, per-level specifications before implementation, documented provenance).
V3 work carries over unchanged; V4 deltas were sent to the running agents.

## Build status
* Linux / GCC 13.3 / Ninja / Release: **builds, 0 project warnings**
* Tests: **12/12 passing** (`ctest --test-dir build`)
* Headless run under Xvfb + Mesa llvmpipe (OpenGL 4.5 core): **renders dev scene, screenshot captured**
* Windows/macOS: not yet verified (code is written portably; CI not available in this environment)

## Completed systems
* Build: CMake project, FetchContent/find_package dependency pipeline, ccache, ASan/UBSan option, CPack packaging skeleton
* Core: logging, assertions, PCG32 RNG + splitmix hashing, coherent noise (gradient/fbm/ridged/cellular, tileable), job system + main-thread queue, crash-safe atomic file writes, generational handles/slot maps, math (AABB, frustum, transform)
* Platform: SDL3 window, GL 4.5/4.3 core context, vsync, fullscreen modes, resolution enumeration
* Input: keyboard/mouse state, rebindable action map (JSON persisted)
* Renderer foundation: RAII GL objects (buffers, textures, framebuffers, VAOs, GPU timers), shader library with `#include`/defines/hot-reload, retained scene API (meshes, materials, instances, lights, environment), baseline forward PBR + ACES tonemap, frame capture
* Settings: full settings model with validation + persistence
* App: fixed-timestep loop with interpolation, CLI options, screenshot mode, benchmark statistics, scene registry
* Profiling: built-in lock-free scope profiler + optional Tracy
* CI: GitHub Actions workflow (Linux GCC Release, Clang Debug+ASan/UBSan under Xvfb, Windows MSVC + package) — defined, not yet observed running
* Docs (V4 layout in `docs/`): architecture, status, log, tests, performance, risk register, references/provenance, content ids

## Incomplete systems
Everything in AGENT_TASKS.md waves 1–4. Physics and audio are currently
**temporary scaffolds** (no simulation / silent) pending wave-1 delivery.

## Known issues
* Baseline renderer has no shadows / indirect light (ceiling renders dark) — addressed by W1-R.
* `XDG_RUNTIME_DIR` warning under Xvfb is benign.

## Last successful test
2026-10-31: `ctest` 11/11; `BackroomsBeyondReality --screenshot` dev scene at 960×540 on llvmpipe.

## Wave-1 agent worktrees (resume points if this session is lost)
| Team | Branch |
|---|---|
| Rendering & Lighting | `worktree-agent-af78e050e9285b6f4` |
| Materials & Tech Art | `worktree-agent-a201c6cec28d2c81f` |
| Physics & Player | `worktree-agent-addb1d47068b4a044` |
| Audio | `worktree-agent-ae2bb4ea2438f4d57` |
| UI/UX + Save | `worktree-agent-a53d3b72512521bbf` |
| Procedural Generation | `worktree-agent-aa178270bc641adf5` |
| Horror & Level Design (docs) | `worktree-agent-af748374b223db8a1` |
Worktrees live under `.claude/worktrees/` (git-ignored). Branches are local until merged.

## Environment notes
After a container restart run `source tools/scripts/dev_env.sh` (restarts Xvfb :99; system packages and
`/home/user/br-deps` persist). Background agents are resumed by the harness automatically.

## Agent policy (since 2026-11-01)
Commit after every working step; report when the core deliverable is tested; polish goes to fresh follow-up agents.
Critical path first: Physics/Player, Generation, Rendering, Materials -> then UI, Audio, Lore.

## Exact next task
Integrate wave-1 branches in order: Physics → Generation → Materials → Rendering → Audio → UI; then wire the
Level 0 vertical slice (menu → load Level 0 → walk, interact, hear, save).

## Important engineering decisions
* Custom engine on SDL3/OpenGL 4.5/Jolt/OpenAL (no game engine).
* Retained-mode renderer API; generators are pure CPU functions producing `ChunkPayload`.
* Jolt always compiled in-tree (config-dependent defines; ODR safety).
* Per-module source globbing to keep parallel work conflict-free.
* All content procedural; no runtime downloads; fonts are OFL (documented).
