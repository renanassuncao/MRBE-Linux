#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/package/mrbe-launcher/src"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
CC="${CC:-cc}"
"$CC" -std=c11 -Os -Wall -Wextra -Werror \
    -DMRBE_LOGO_PATH=\"$SRC/assets/brasao-256.rgba\" \
    -o "$TMP/mrbe-preview" "$SRC/preview.c" "$SRC/ui.c" "$SRC/fb.c" "$SRC/font.c"

if [ "${MRBE_PREVIEW_QUICK:-0}" = "1" ]; then
    screens=(home apps store settings)
else
    screens=(home apps store library details films series sports kids search settings)
fi
for view in "${screens[@]}"; do
    "$TMP/mrbe-preview" "$TMP/${view}.ppm" "$view"
    if python3 -c 'from PIL import Image' >/dev/null 2>&1; then
        python3 - "$TMP/${view}.ppm" "$ROOT/docs/preview-mrbe-${view}.png" <<'PYTHON'
import sys
from PIL import Image
Image.open(sys.argv[1]).save(sys.argv[2], optimize=True)
print('Previa PNG:', sys.argv[2])
PYTHON
    else
        cp "$TMP/${view}.ppm" "$ROOT/docs/preview-mrbe-${view}.ppm"
        echo "Previa PPM: docs/preview-mrbe-${view}.ppm (python3-pil para PNG)"
    fi
done
