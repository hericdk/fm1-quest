#!/bin/sh
# Build web/fm1quest.wasm: the FM1 Quest firmware of this repository + the web HAL (src/fm1quest_wasm.c, from
# sabliran/sloop-web-emu).
#   needs: clang + wasm-ld (or zig: `pip install ziglang`, then CC=zig), python3 + numpy + pillow
#   ./build.sh                 # the firmware defaults to the repository root (..)
set -e
cd "$(dirname "$0")"
SLOOP=${SLOOP:-$(cd .. && pwd)}
CC=${CC:-clang}
TARGET=wasm32
if [ "$CC" = zig ]; then CC="python3 -m ziglang cc"; TARGET=wasm32-freestanding; fi
# the generated headers (fonts, icons, tables, samples, the Quest art and its LZ4 packing): the same step as the firmware build
python3 -c "import sys; sys.path.insert(0, '$SLOOP/tools'); import build; build.generate()"
$CC --target=$TARGET -O2 -fno-builtin -ffreestanding -nostdlib -w -s \
  -I "$SLOOP/build/gen" -I "$SLOOP/firmware/src" -I src \
  -Wl,--no-entry -Wl,--export-memory -Wl,-z,stack-size=1048576 -Wl,--global-base=1048576 \
  -o web/fm1quest.wasm src/fm1quest_wasm.c
ls -la web/fm1quest.wasm
