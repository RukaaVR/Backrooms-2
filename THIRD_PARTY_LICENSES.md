# Third-Party Software and Assets

All third-party code is either fetched at configure time from its official
repository at a pinned tag (`cmake/Dependencies.cmake`) or vendored under
`third_party/` with its license. No runtime asset is downloaded from the
network; every texture, mesh and sound in the game is generated procedurally
by project code unless listed here.

| Component | Version | License | Source | Use |
|---|---|---|---|---|
| SDL3 | 3.2.10 | zlib | https://github.com/libsdl-org/SDL | windowing, input, GL context |
| Jolt Physics | 5.3.0 | MIT | https://github.com/jrouwe/JoltPhysics | rigid-body physics, character controller |
| OpenAL Soft | system / 1.24.2 | LGPL-2.1 (dynamically linked) | https://github.com/kcat/openal-soft | 3D audio |
| GLM | 1.0.1 | MIT / Happy Bunny | https://github.com/g-truc/glm | math |
| nlohmann/json | 3.11.3 | MIT | https://github.com/nlohmann/json | settings/saves |
| Catch2 | 3.7.1 | BSL-1.0 | https://github.com/catchorg/Catch2 | tests only |
| stb_image, stb_image_write, stb_truetype, stb_rect_pack | commit 003501f | MIT / Public Domain | https://github.com/nothings/stb | image IO, font rasterisation (`third_party/stb/LICENSE`) |
| glad2 generated GL 4.5 loader | glad 2.x | generated code: MIT/Apache-2.0 (Khronos XML) | https://github.com/Dav1dde/glad | OpenGL function loading |

## Fonts (`assets/fonts/`)
| Font | License | Provenance |
|---|---|---|
| Inter (Regular, Light, SemiBold) 4.0 | SIL Open Font License 1.1 | Debian package `fonts-inter` 4.0+ds-1; license text in `assets/fonts/LICENSE-Inter.txt` |
| JetBrains Mono Bold | SIL OFL 1.1 / Apache-2.0 (per Debian copyright) | Debian package `fonts-jetbrains-mono`; `assets/fonts/LICENSE-JetBrainsMono.txt` |

OpenAL Soft is LGPL: it is linked dynamically so users may replace the library.
