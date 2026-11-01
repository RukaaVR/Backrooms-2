# Architecture — THE BACKROOMS: BEYOND REALITY

Custom native C++20 engine + game. No third-party game engine is used; the only
dependencies are low-level libraries (see `LICENSES/THIRD_PARTY_LICENSES.md`).

## Technology

| Concern        | Choice                                   | Notes |
|----------------|------------------------------------------|-------|
| Language/build | C++20, CMake ≥ 3.24, Ninja               | GCC 13 / Clang 18 / MSVC 2022 |
| Platform       | SDL3 3.2.10                              | window, GL context, input, paths |
| Rendering      | OpenGL 4.5 core (4.3 fallback), GLSL 450 | glad2-generated loader (vendored) |
| Physics        | Jolt Physics 5.3.0                       | built in-tree (see below) |
| Audio          | OpenAL Soft (+EFX)                       | system lib on Linux, source build elsewhere |
| Math           | GLM 1.0.1                                | |
| Images/fonts   | stb_image / stb_image_write / stb_truetype | vendored |
| Serialization  | nlohmann/json 3.11.3                     | |
| Tests          | Catch2 3.7.1                             | |

### Why Jolt is always built in-tree
Jolt exports configuration-dependent compile definitions (asserts, debug
renderer, profiler) that must match every consuming translation unit. A
prebuilt Release Jolt linked into a Debug game is an ODR violation that
manifests as heap corruption. Building it in-tree guarantees matching flags;
ccache makes it cheap.

## Layout

```
engine/        reusable engine library (br_engine) — no Backrooms knowledge
  core/        logging, asserts, RNG/hash, noise, job system, file system, handles, math
  platform/    SDL3 window + GL context
  input/       raw input + rebindable action map
  renderer/    GL RAII wrappers, shader library, retained-mode Renderer
  materials/   material registry (+ procedural texture framework)
  physics/     Jolt facade: PhysicsWorld, CharacterController
  audio/       OpenAL Soft mixer, procedural synthesis
  scene/       CPU mesh data + procedural primitive builders
  resources/   images
  streaming/   generic chunk streaming
  ui/          immediate-mode UI renderer, fonts, widgets
  shaders/     GLSL (copied to <bin>/data/shaders)
game/          Backrooms game library (br_game) + executable
  app/         Game (main loop, state machine), launch options, dev scene
  settings/    persisted settings
  world/       ChunkPayload contract, world streamer, prefabs
  levels/      level registry + per-level definitions/generators
  player/ camera/ interaction/ inventory/ survival/ entities/ ai/ horror/
  progression/ water/ save/ ui/ audio/ materials/
assets/        runtime assets (fonts, generated caches) -> <bin>/data/assets
tools/         developer tools (validators, generators, scripts)
tests/         Catch2 tests (engine/, physics/, generation/, gameplay/, integration/)
```

Include paths are rooted at the repository: `#include "engine/core/Log.h"`.
Each library globs its own sources (`CONFIGURE_DEPENDS`) so parallel feature
work never conflicts on a central source list.

## Runtime data layout
```
<bin>/BackroomsBeyondReality
<bin>/data/shaders/...
<bin>/data/assets/...
<user>/settings.json, saves/, cache/, screenshots/, backrooms.log
```
`<user>` = `SDL_GetPrefPath()` (overridable with `--user-dir`). No absolute or
hardcoded paths anywhere; see `br::Paths`.

## Frame loop (game/app/Game.cpp)

```
pumpEvents()                      SDL events -> Window, InputSystem, UI
fixedUpdate(1/60 s) x N           physics step, player controller, AI, gameplay
update(dt, alpha)                 main-thread queue (GPU uploads), streaming,
                                  camera (interpolated), audio, horror director
render(dt)                        Renderer::renderFrame -> UI on top -> swap
```
Simulation runs at a fixed 60 Hz; rendering interpolates with `alpha`.

## Threading
* Main thread owns GL, SDL, OpenAL and Jolt body creation/destruction.
* `JobSystem` workers run chunk generation, procedural texture/sound synthesis,
  nav baking. Workers never touch GL; results return via `MainThreadQueue`
  (drained with a per-frame time budget to avoid hitches).
* Generators are pure functions of `(level seed, chunk coord)`.

## Key contracts
| Contract | File | Producer -> Consumer |
|---|---|---|
| `render::Renderer` retained API | engine/renderer/Renderer.h | everyone -> rendering |
| `render::MaterialDesc/LightDesc/EnvironmentDesc` | engine/renderer/RenderTypes.h | materials, levels -> renderer |
| `materials::MaterialRegistry` | engine/materials/MaterialRegistry.h | material library -> world streamer |
| `physics::PhysicsWorld`, `CharacterController` | engine/physics/*.h | physics -> gameplay |
| `audio::AudioSystem` | engine/audio/AudioSystem.h | audio -> gameplay |
| `world::ChunkPayload` | game/world/ChunkData.h | level generators -> world streamer |
| `levels::LevelDefinition`, `ChunkGenerator` | game/levels/LevelDefinition.h | levels -> game |
| `game::Settings` | game/settings/Settings.h | UI <-> all systems |
| `game::GameContext` | game/app/GameContext.h | Game -> feature systems |

Contracts may be **extended** by their owner; breaking changes require the
Technical Director's integration.

## Rendering pipeline (target)
Linear-light HDR deferred pipeline: G-buffer -> SSAO -> clustered lighting
(rect/point/spot lights, shadow atlas, bounce-light approximation) ->
volumetric fog -> forward transparents/water -> SSR -> TAA -> bloom ->
auto exposure -> tonemap + grading + grain -> UI. See `docs/RENDERING.md`.

## Determinism
All procedural content derives from `br::Rng` (PCG32) and `hash2i/hash3i`
(splitmix64). Golden-value tests guard against accidental changes.

## Profiling
`engine/core/Profiler.h`: `BR_PROFILE_SCOPE("name")` / `BR_PROFILE_FRAME()`. Default
backend is a lock-free per-call-site aggregator (two relaxed atomics per scope) read once
per frame for the debug overlay and benchmarks. `-DBR_ENABLE_TRACY=ON` routes the same
macros to Tracy v0.11.1 for live capture.

## Deviations from the suggested V4 stack/layout (and why)
| Suggestion | Decision | Reason |
|---|---|---|
| spdlog | Custom `br::log` (std::format, thread-safe, file + stderr) | Covers all needs in ~100 lines; avoids a dependency (V4: "do not introduce unnecessary dependencies") |
| Assimp | Not used | All meshes are generated procedurally in code; no model files to import |
| Dear ImGui | Planned for **developer tools only** (debug overlay, tuning panels) in the QA/tools wave; the game UI is custom (`engine/ui`) | Keeps shipping UI fully custom while giving developers fast tooling |
| Tracy | Optional (`BR_ENABLE_TRACY`) | Zero cost when off; built-in profiler always available |
| `levels/level_000/...` top-level | `game/levels/levelN/` | Level code depends on game systems (generation toolkit, prefabs, entities); keeping it inside `br_game` avoids a third library and circular dependencies. Ownership per level directory is preserved |
| Status docs at repo root (V3) | Moved to `docs/` (V4) | Latest brief wins; README links them |
