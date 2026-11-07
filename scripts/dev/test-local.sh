#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
echo '[1/5] Sintaxe scripts bash'
while IFS= read -r -d '' f; do bash -n "$f"; done < <(find scripts -type f -name '*.sh' -print0)
echo '[2/5] Sintaxe scripts sh'
sh -n board/qemu-aarch64/rootfs-overlay/etc/init.d/S05mrbe-launcher
sh -n board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe
sh -n board/qemu-aarch64/rootfs-overlay/usr/bin/mrbe-info
sh -n board/qemu-aarch64/rootfs-overlay/usr/bin/mrbe-diagnostico
echo '[3/5] C nativo, sem erros de compilacao'
cc -std=c11 -Wall -Wextra -Werror -fsyntax-only package/mrbe-launcher/src/{main,fb,font,input,ui,preview}.c
echo '[4/5] Documentos, config e JSON'
python3 -m unittest discover -s tests -p 'test_*.py' -v
echo '[5/5] Binario host + preview'
./scripts/dev/render-preview.sh
echo '[APROVADO] Testes estaticos concluídos (NAO substituem o boot em QEMU)'
