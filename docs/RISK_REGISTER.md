# Risk Register

Owner: Technical Director. Reviewed at every integration milestone.
Likelihood/Impact: L(ow) / M(edium) / H(igh).

| # | Risk | L | I | Mitigation | Status |
|---|---|---|---|---|---|
| R1 | No GPU in the development container: real-hardware performance cannot be measured here; llvmpipe numbers are meaningless for 1080p60 claims | H | H | Performance designed in (culling, instancing, cached shadows, half-res effects, quality tiers); CI artefacts + `--benchmark` mode so measurements can be taken on real hardware; `PERFORMANCE.md` never reports llvmpipe timings as targets | Open |
| R2 | Usage/rate limits interrupt parallel agents mid-task | H | M | Agents work in isolated worktrees and commit frequently; status docs record exact next task; resumable agents | Occurred once (2026-10-31); mitigated |
| R3 | Merge conflicts between parallel teams | M | M | Strict module ownership, per-module source globbing, contract headers extended additively only, scene registry table, TD-only integration | Open |
| R4 | Windows/macOS builds untested (Linux-only container) | M | H | Portable code (SDL3/OpenGL/OpenAL), MSVC flags in CMake, Windows CI job defined in `.github/workflows/ci.yml`; first CI run must be checked | Open |
| R5 | Photorealism target vs procedural-only content (no scanned/photo textures) | H | H | Invest in procedural material framework with multi-scale variation; reference-driven palette calibration (`REFERENCES.md`); lighting quality (area lights, GI approximation) carries realism | Open |
| R6 | "Exact layout" fidelity: Backrooms canon rarely provides measurable maps; commercial games' maps are proprietary | H | M | Authored canonical layouts as *faithful interpretations* from public lore + user-supplied references; provenance and verified/unverified tables per level (`LEVEL_SPECIFICATIONS.md`) | Open |
| R7 | Entity visuals/animation without an asset pipeline (no DCC models) | M | H | Procedural meshes + code-driven skeletal animation; lean on darkness, silhouette, sound and behaviour (horror benefits from not showing too much) | Open |
| R8 | OpenGL driver differences (compute, SSBO limits, extensions) | M | M | Capability detection with logged fallbacks (renderer task); GL 4.3 minimum enforced at startup | Open |
| R9 | Streaming hitches (GPU uploads / Jolt broadphase insertion) | M | M | Time-budgeted main-thread activation, batched static body insertion, async generation | Open |
| R10 | Scope: 16 levels at high fidelity | H | H | Fidelity over quantity (V4): Level 0 first to benchmark quality; levels only marked complete after architecture/material/lighting/gameplay/navigation/audio/performance review | Open |
| R11 | Third-party licensing (LGPL OpenAL, fonts) | L | M | Dynamic linking of OpenAL Soft; OFL fonts with licence texts in `LICENSES/`; no external audio/textures | Mitigated |
| R12 | Save corruption / data loss | L | H | Versioned saves, checksums, atomic replace, rotating backups, migration tests (UI/Save team) | In progress |
