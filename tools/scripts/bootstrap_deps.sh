#!/usr/bin/env bash
# Pre-fetches and pre-builds third-party dependencies into a shared prefix so that
# multiple build trees (Debug/Release/ASan, git worktrees, CI jobs) do not each
# rebuild SDL3/Catch2 from scratch. This is an optimisation only: a plain CMake
# configure without this script falls back to FetchContent automatically.
#
# Usage: tools/scripts/bootstrap_deps.sh [deps-root]
#   deps-root defaults to $HOME/.cache/backrooms-deps
# Afterwards configure with:
#   cmake -B build -G Ninja -DBR_DEPS_ROOT=<deps-root>
set -euo pipefail

DEPS_ROOT="${1:-${HOME}/.cache/backrooms-deps}"
SRC="${DEPS_ROOT}/src"
PREFIX="${DEPS_ROOT}/prefix"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
mkdir -p "${SRC}" "${PREFIX}"

clone() { # name url tag
  local name="$1" url="$2" tag="$3"
  if [[ ! -d "${SRC}/${name}/.git" ]]; then
    git clone --depth 1 --branch "${tag}" "${url}" "${SRC}/${name}"
  fi
}

# Versions must stay in sync with cmake/Dependencies.cmake.
clone sdl3     https://github.com/libsdl-org/SDL.git        release-3.2.10
clone jolt     https://github.com/jrouwe/JoltPhysics.git    v5.3.0
clone glm      https://github.com/g-truc/glm.git            1.0.1
clone catch2   https://github.com/catchorg/Catch2.git       v3.7.1
clone json     https://github.com/nlohmann/json.git         v3.11.3

build_install() { # name [cmake args...]
  local name="$1"; shift
  local bdir="${DEPS_ROOT}/build/${name}"
  if [[ -f "${bdir}/.installed" ]]; then return; fi
  cmake -S "${SRC}/${name}" -B "${bdir}" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_POSITION_INDEPENDENT_CODE=ON "$@"
  cmake --build "${bdir}" -j "${JOBS}"
  cmake --install "${bdir}"
  touch "${bdir}/.installed"
}

build_install sdl3 -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF -DSDL_TESTS=OFF
build_install glm -DGLM_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
build_install catch2 -DCATCH_INSTALL_DOCS=OFF -DCATCH_INSTALL_EXTRAS=ON
build_install json -DJSON_BuildTests=OFF -DJSON_Install=ON

echo "Dependencies ready in ${DEPS_ROOT}"
echo "Configure with: cmake -B build -G Ninja -DBR_DEPS_ROOT=${DEPS_ROOT}"
