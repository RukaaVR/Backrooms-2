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
