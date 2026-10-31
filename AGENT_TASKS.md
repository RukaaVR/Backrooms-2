# Agent Task Board

The Technical Director (main session) owns architecture, contracts, integration
and review. Sub-agents work in isolated git worktrees on disjoint file sets and
deliver a branch + completion report. Nothing is merged without the director
building it, running the tests and inspecting output.

Status legend: `TODO` · `IN PROGRESS` · `REVIEW` · `DONE` · `BLOCKED`

## Module ownership

| Agent | Role | Owns (exclusive write access) |
|---|---|---|
| 01 | Technical Director | `CMakeLists.txt`, `cmake/`, `engine/core/`, `engine/platform/`, `engine/input/`, `game/app/`, contracts (`game/world/ChunkData.h`, `game/levels/LevelDefinition.h`, `game/app/GameContext.h`), all `*.md` status docs |
| 02+03 | Rendering & Lighting | `engine/renderer/` (except contract types may only be extended), `engine/shaders/`, `docs/RENDERING.md` |
| 03+06 | Materials & Technical Art | `engine/materials/` (except MaterialRegistry contract), `game/materials/`, `tools/asset_generator/`, `docs/MATERIALS.md` |
| 04+07 | Physics & Player | `engine/physics/`, `game/player/`, `game/camera/`, `game/interaction/`, `tests/physics/` |
| 05+06 | Procedural Generation & Streaming | `engine/streaming/`, `game/world/` (except ChunkData.h), `game/generation/`, `game/levels/level0/`, `tools/level_validator/`, `tests/generation/` |
| 09 | Audio | `engine/audio/`, `game/audio/`, `tests/audio/` |
| 10 | UI/UX + Save | `engine/ui/`, `game/ui/`, `game/save/`, `tests/ui/`, `tests/save/` |
| 08 | AI | `game/ai/`, `game/entities/` |
| 07 | Gameplay | `game/inventory/`, `game/survival/`, `game/progression/` |
| 11 | Horror & Level Design | `game/horror/`, `game/levels/levelN/` (non-0), `docs/LORE.md`, `docs/LEVEL_DESIGN.md` |
| 12 | QA & Optimisation | `tests/integration/`, `tools/screenshot_harness/`, `tools/profiling/`, `TEST_RESULTS.md` inputs |

## Wave 0 — Foundation (Technical Director) — DONE
* CMake, dependency pipeline, vendored glad/stb/fonts
* core (log, RNG, noise, jobs, fs, handles, math), platform, input + bindings
* GPU RAII wrappers, shader library, baseline forward renderer, screenshot capture
* contracts: Renderer API, physics/audio interfaces (temporary scaffolds), ChunkPayload, LevelDefinition, Settings
* Catch2 tests (11), headless render verified under Xvfb/llvmpipe

## Wave 1 — Level 0 vertical slice (parallel)

| ID | Agent | Task | Status |
|---|---|---|---|
| W1-R | Rendering & Lighting | Deferred HDR pipeline, clustered rect/point/spot lights, shadow atlas + PCF, SSAO, bounce-light GI approximation, volumetric fog, SSR, TAA/FXAA, bloom, auto-exposure, grading/grain, transparent pass hook | IN PROGRESS |
| W1-M | Materials & Tech Art | Procedural PBR texture framework (threaded, cached), Level 0 material set at photographic quality, materials for every other level | IN PROGRESS |
| W1-P | Physics & Player | Jolt PhysicsWorld + CharacterController, PlayerController (movement feel, crouch, stairs, camera bob), interaction (raycast, pick up/carry/throw, hinged doors), buoyancy | IN PROGRESS |
| W1-A | Audio | OpenAL Soft mixer + EFX reverb/low-pass, procedural synthesis bank (hum, footsteps per surface, impacts, ambience beds, entity sounds), soundscape system | IN PROGRESS |
| W1-U | UI/UX + Save | SDF font + UI renderer, widgets, main/pause/settings/level-select/extras menus, HUD, versioned atomic save system | IN PROGRESS |
| W1-G | Procedural Generation | Chunk streamer, WorldStreamer (payload -> renderer/physics), Level 0 generator, connectivity validator + map dump, tests | IN PROGRESS |

## Wave 2 — Gameplay depth (planned)
* AI: navigation (grid A* from NavGrid), perception, FSM; Hounds, Smilers, Facelings, Skin-Stealers, Partygoers
* Gameplay: inventory, items (flashlight, batteries, almond water, keys, documents, med kits), survival stats, checkpoints, no-clip transitions, progression
* Horror director, environmental events
* Water system (surface rendering, swimming, underwater fog/audio) for Levels 7 and 37
* Props/prefab library (furniture, office, hotel, industrial)

## Wave 3 — Levels (planned)
Levels 1–11, 37, 94, 188, Fun — each a specialised generator + materials + soundscape + entity rules.

## Wave 4 — QA, optimisation, polish, packaging (planned)
Screenshot suite, sanitizer runs, profiling, performance tuning, visual polish, CPack packages.
