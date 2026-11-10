#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
echo '[1/6] Sintaxe scripts bash'
while IFS= read -r -d '' f; do bash -n "$f"; done < <(find scripts -type f -name '*.sh' -print0)
echo '[2/6] Sintaxe scripts sh do rootfs'
while IFS= read -r -d '' f; do sh -n "$f"; done < <(find board/qemu-aarch64/rootfs-overlay -type f -path '*/usr/bin/*' -print0)
sh -n board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe
echo '[3/6] C nativo, sem erros de compilacao'
cc -std=c11 -Wall -Wextra -Werror -fsyntax-only package/mrbe-launcher/src/{main,fb,font,input,ui,preview}.c
echo '[4/6] Documentos, config e JSON + testes API locais'
python3 -m unittest discover -s tests -p 'test_*.py' -v
echo '[5/6] Verificacao de configuracao Buildroot e scripts API'
grep -q '^BR2_PACKAGE_LIBCURL_CURL=y$' configs/mrbe_qemu_aarch64_defconfig
grep -q '^BR2_PACKAGE_OPENSSL=y$' configs/mrbe_qemu_aarch64_defconfig
grep -q '^BR2_PACKAGE_CA_CERTIFICATES=y$' configs/mrbe_qemu_aarch64_defconfig
echo '[6/6] Binario host + preview'
MRBE_PREVIEW_QUICK=1 ./scripts/dev/render-preview.sh
echo '[APROVADO] Testes estaticos concluídos (NAO substituem o boot em QEMU)'
