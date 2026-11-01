# Development Log

## 2026-10-31 — Session 1

### Environment inspection
* Ubuntu 24.04, 4 cores, 15 GB RAM. GCC 13.3, Clang 18.1, CMake 3.28, Ninja, ccache.
* GPU: none; Mesa llvmpipe provides OpenGL 4.5 core under Xvfb — sufficient for
  automated rendering tests and screenshots (performance numbers from llvmpipe
  are NOT representative of real GPUs and are never reported as such).
* Network: GitHub reachable via git; dependencies pinned and cloned.

### Wave 0 (Technical Director)
* Built the dependency pipeline (`cmake/Dependencies.cmake`,
  `tools/scripts/bootstrap_deps.sh`). Decision: Jolt in-tree (ODR safety).
* Vendored glad2 (GL 4.5 core + KHR_debug + anisotropic filtering), stb, OFL fonts.
* Implemented engine core, platform, input, GPU wrappers, shader library,
  retained renderer API with a baseline forward PBR path.
* Defined cross-team contracts (Renderer, PhysicsWorld, CharacterController,
  AudioSystem, ChunkPayload, LevelDefinition, Settings, GameContext).
* Physics/audio ship as clearly marked temporary scaffolds so every worktree
  links while the real implementations are developed in parallel.
* Verified: build clean, 11/11 tests, headless screenshot of the dev scene.

### Wave 1 launched
Six sub-agents in isolated worktrees: Rendering & Lighting, Materials & Tech
Art, Physics & Player, Audio, UI/UX + Save, Procedural Generation & Streaming.

### V4 brief adopted (2026-10-31 → 11-01)
* User supplied MASTER PROMPT V4 + five reference images (Level 0 renders, the original
  photo, a decayed variant, a warehouse photo for Level 1). Continued rather than restarted:
  the foundation already matches V4's stack and process.
* References converted and kept outside the repo (unknown provenance); palette measured
  (greenish yellow: G ≥ R, B ≈ 0.6R; ceiling lit by bounce) — see `REFERENCES.md`.
* V4 deltas sent to all seven running agents (see AGENT_TASKS).
* Moved project-memory docs into `docs/`, licences into `LICENSES/`; added risk register,
  performance report (no measurements yet — no GPU here), references/provenance.
* Added built-in profiler + optional Tracy; instrumented the main loop; profiler test.
* Usage limit interrupted all agents; resumed after reset from their saved worktrees.

### 2026-11-01 — second usage-limit interruption; convergence strategy
* All seven agents were stopped by the 5-hour usage limit a second time. Committed so far:
  Physics (Jolt PhysicsWorld + CharacterVirtual controller), Audio (OpenAL mixer, EFX
  presets, DSP toolkit), UI (SDF fonts with kerning, UI renderer, keybind capture),
  Rendering (foundation: ring buffers, mesh pool, GPU profiler, struct mirrors), Lore
  (LORE_REFERENCE.md). Materials and Generation had substantial uncommitted work.
* Lesson: very long agent transcripts burn the usage window. New policy: agents commit
  after every working step and report as soon as their core deliverable is tested;
  polish goes to fresh, focused follow-up agents (smaller context, cheaper per step).
* Resumed the critical path for the Level 0 vertical slice first: Physics/Player,
  Generation, Rendering, Materials. UI, Audio and Lore resume after these land.
