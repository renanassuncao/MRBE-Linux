#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/work/output"
fail=0
for f in "$OUT/.config" "$OUT/images/Image" "$OUT/images/rootfs.ext4"; do
    if [ -s "$f" ]; then ls -lh "$f"; else echo "[ERRO] Artefato ausente: $f"; fail=1; fi
done
if [ -s "$OUT/.config" ]; then
    for key in 'BR2_aarch64=y' 'BR2_TOOLCHAIN_BUILDROOT_MUSL=y' 'BR2_PACKAGE_MRBE_LAUNCHER=y' 'BR2_PACKAGE_LIBCURL=y' 'BR2_PACKAGE_LIBCURL_CURL=y' 'BR2_PACKAGE_OPENSSL=y' 'BR2_PACKAGE_LIBCURL_OPENSSL=y' 'BR2_PACKAGE_CA_CERTIFICATES=y'; do
        grep -q -F "$key" "$OUT/.config" || { echo "[ERRO] Opcao Buildroot: $key"; fail=1; }
    done
fi
ASSET_DIR="$OUT/target/usr/share/mrbe/ui"
if [ -d "$ASSET_DIR" ]; then
    asset_count="$(find "$ASSET_DIR" -maxdepth 1 -type f -name '*.rgba' | wc -l)"
    if [ "$asset_count" -ge 31 ]; then
        echo "[MRBE] Assets da Home instalados: $asset_count"
    else
        echo "[ERRO] Assets da Home incompletos: $asset_count/31 em $ASSET_DIR"
        fail=1
    fi
else
    echo "[ERRO] Diretorio de assets ausente: $ASSET_DIR"
    fail=1
fi

TARGET="$OUT/target"
for utility in mrbe-api-test mrbe-healthcheck mrbe-stability-test; do
    if [ -x "$TARGET/usr/bin/$utility" ]; then
        echo "[MRBE] Utilitario instalado: $utility"
    else
        echo "[ERRO] Utilitario ausente: $TARGET/usr/bin/$utility"
        fail=1
    fi
done
if [ -x "$TARGET/usr/bin/curl" ]; then
    echo '[MRBE] Cliente curl instalado'
else
    echo '[ERRO] curl ausente; API HTTPS nao pode ser testada'
    fail=1
fi
if [ -s "$TARGET/etc/ssl/certs/ca-certificates.crt" ]; then
    echo '[MRBE] Bundle de certificados CA instalado'
else
    echo '[ERRO] Bundle CA ausente; validacao TLS indisponivel'
    fail=1
fi

if [ "$fail" -ne 0 ]; then exit 1; fi
echo '[MRBE] Artefatos presentes. Isso NAO comprova o boot: execute make smoke.'
