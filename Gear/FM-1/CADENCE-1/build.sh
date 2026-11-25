#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
set -eu
cd "$(dirname "$0")"

base_rev=3c4a57eea659f942b0c80a99949dddafc094a57b
bsp_rev=41a6e86d921f842243bd01a48111832f7e55b4e2
local_base=${FM1_LOCAL_BASE:-"$HOME/fm1/trio/base"}
if [ ! -e base/tools/build.py ]; then
    tmp=$(mktemp -d .base.XXXXXXXX)
    trap 'rm -rf "$tmp"' EXIT HUP INT TERM
    if git -C "$local_base" cat-file -e "$base_rev^{commit}" 2>/dev/null &&
       git -C "$local_base/bsp" cat-file -e "$bsp_rev^{commit}" 2>/dev/null; then
        git -C "$local_base" archive "$base_rev" -o "$PWD/$tmp/base.tar"
        git -C "$local_base/bsp" archive "$bsp_rev" -o "$PWD/$tmp/bsp.tar"
        mkdir "$tmp/src"
        tar -xf "$tmp/base.tar" -C "$tmp/src"
        mkdir -p "$tmp/src/bsp"
        tar -xf "$tmp/bsp.tar" -C "$tmp/src/bsp"
    else
        git clone -q https://github.com/charlesvestal/fm1-base "$tmp/src"
        git -C "$tmp/src" checkout -q --detach "$base_rev"
        git -C "$tmp/src" -c submodule.bsp.url=https://github.com/hugelton/felucca-bsp submodule update --init
    fi
    mv "$tmp/src" base
    rm -rf "$tmp"
    trap - EXIT HUP INT TERM
fi

export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
if [ -z "${PYTHON:-}" ] && [ -x "$HOME/fm1/venv/bin/python" ]; then
    PYTHON="$HOME/fm1/venv/bin/python"
fi
exec "${PYTHON:-python3}" base/tools/build.py app "$@"
