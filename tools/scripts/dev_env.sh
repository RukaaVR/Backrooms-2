#!/usr/bin/env bash
# Restores the headless development environment after a container restart:
# starts Xvfb on :99 (OpenGL 4.5 core via Mesa llvmpipe) if it is not running
# and reports the toolchain state. Source it to also export DISPLAY:
#   source tools/scripts/dev_env.sh
set -u
if ! pgrep -x Xvfb >/dev/null 2>&1; then
    setsid nohup Xvfb :99 -screen 0 1920x1080x24 -nolisten tcp >/tmp/xvfb.log 2>&1 </dev/null &
    sleep 1
fi
export DISPLAY=:99
echo "DISPLAY=$DISPLAY ($(pgrep -x Xvfb >/dev/null && echo Xvfb running || echo Xvfb NOT running))"
command -v glxinfo >/dev/null && glxinfo -B 2>/dev/null | grep -m1 "core profile version" || true
for tool in cmake ninja ccache g++; do
    command -v "$tool" >/dev/null || echo "missing tool: $tool"
done
[ -d "${HOME}/br-deps/prefix" ] || [ -d /home/user/br-deps/prefix ] || echo "prebuilt deps missing: run tools/scripts/bootstrap_deps.sh /home/user/br-deps"
