#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="$ROOT/work"
BR_VERSION=2025.02.19
BR_ARCHIVE="$WORK/buildroot-${BR_VERSION}.tar.xz"
BR_SRC="$WORK/buildroot-${BR_VERSION}"
OUT="$WORK/output"
DL="$WORK/dl"
JOBS="${JOBS:-4}" # evita estourar RAM no WSL com muitos nucleos
if ! [[ "$JOBS" =~ ^[1-9][0-9]*$ ]]; then echo 'JOBS deve ser numero positivo'; exit 2; fi
mkdir -p "$WORK" "$DL"
# Buildroot rejects Windows PATH entries with spaces (common under WSL).
# GNU install is required on Ubuntu releases that ship uutils install.
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
if [ -x /usr/bin/gnuinstall ]; then
    mkdir -p "$WORK/host-tools"
    ln -sfn /usr/bin/gnuinstall "$WORK/host-tools/install"
    export PATH="$WORK/host-tools:$PATH"
fi
chmod +x "$ROOT/board/qemu-aarch64/rootfs-overlay/etc/init.d/S05mrbe-launcher" "$ROOT/board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe"  "$ROOT/board/qemu-aarch64/rootfs-overlay/usr/bin/mrbe-diagnostico"  "$ROOT/board/qemu-aarch64/rootfs-overlay/usr/bin/mrbe-info"
for tool in make tar sha256sum; do command -v "$tool" >/dev/null || { echo "Dependencia ausente: $tool"; exit 2; }; done
if [ ! -f "$BR_SRC/Makefile" ]; then
    echo "[MRBE] Obtendo Buildroot LTS $BR_VERSION"
    if [ ! -s "$BR_ARCHIVE" ]; then
        if command -v curl >/dev/null; then
            curl -fL --retry 3 --connect-timeout 20 "https://buildroot.org/downloads/buildroot-${BR_VERSION}.tar.xz" -o "$BR_ARCHIVE.partial"
        elif command -v wget >/dev/null; then
            wget -O "$BR_ARCHIVE.partial" "https://buildroot.org/downloads/buildroot-${BR_VERSION}.tar.xz"
        else
            echo 'Instale curl ou wget'; exit 2
        fi
        mv "$BR_ARCHIVE.partial" "$BR_ARCHIVE"
    fi
    tar -tJf "$BR_ARCHIVE" >/dev/null || { echo 'Arquivo Buildroot invalido; remova o tar.xz e tente de novo'; exit 2; }
    tar -xJf "$BR_ARCHIVE" -C "$WORK"
fi
# olddefconfig conserva o .config antigo: uma mudanca no defconfig ou num
# fragment nunca chegaria a quem ja compilou. Com o hash diferente, o defconfig
# e reaplicado, kernel e BusyBox sao reconfigurados e o rootfs e reinstalado do
# zero (some o que a config nova removeu); toolchain e pacotes compilados ficam.
CONFIG_HASH="$(cd "$ROOT" && find configs board/qemu-aarch64 -maxdepth 1 -type f \( -name '*defconfig' -o -name '*.fragment' -o -name '*.config' \) -print0 | sort -z | xargs -0 sha256sum | sha256sum | cut -d ' ' -f 1)"
CONFIG_STAMP="$WORK/.mrbe-config.sha256"
RECONFIGURE=()
if [ ! -f "$OUT/.config" ]; then
    echo '[MRBE] Configurando Buildroot (primeira execucao)'
    make -C "$BR_SRC" O="$OUT" BR2_DL_DIR="$DL" BR2_EXTERNAL="$ROOT" mrbe_qemu_aarch64_defconfig
elif [ "$CONFIG_HASH" != "$(cat "$CONFIG_STAMP" 2>/dev/null || true)" ]; then
    echo '[MRBE] defconfig ou fragment mudou: reaplicando o defconfig (ajustes manuais no .config se perdem).'
    make -C "$BR_SRC" O="$OUT" BR2_DL_DIR="$DL" BR2_EXTERNAL="$ROOT" mrbe_qemu_aarch64_defconfig
    for pkg in linux busybox; do
        if compgen -G "$OUT/build/$pkg-[0-9]*" >/dev/null; then RECONFIGURE+=("$pkg-reconfigure"); fi
    done
    if [ -d "$OUT/target" ]; then
        rm -rf "$OUT/target"
        find "$OUT/build" -name .stamp_target_installed -delete
    fi
else
    echo '[MRBE] Configuracao existente. Executando olddefconfig.'
    make -C "$BR_SRC" O="$OUT" BR2_DL_DIR="$DL" BR2_EXTERNAL="$ROOT" olddefconfig
fi
# Buildroot caches packages using stamp files. A source update must explicitly
# invalidate the local launcher package or old 0.2 UI can remain in rootfs.
SRC_HASH="$(find "$ROOT/package/mrbe-launcher/src" -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | cut -d ' ' -f 1)"
STAMP="$WORK/.mrbe-launcher-source.sha256"
PREVIOUS_HASH="$(cat "$STAMP" 2>/dev/null || true)"
if [ -d "$OUT/build" ] && { [ "$SRC_HASH" != "$PREVIOUS_HASH" ] || [ "${MRBE_FORCE_LAUNCHER_REBUILD:-0}" = '1' ]; }; then
    echo '[MRBE] Interface modificada: invalidando somente o launcher (preservando kernel e toolchain).'
    make -C "$BR_SRC" O="$OUT" BR2_DL_DIR="$DL" BR2_EXTERNAL="$ROOT" mrbe-launcher-dirclean
fi
printf '%s\n' "[MRBE] Compilando com JOBS=$JOBS; log em $WORK/build.log"
# pipefail conserva o codigo de erro original do make
make -C "$BR_SRC" O="$OUT" BR2_DL_DIR="$DL" BR2_EXTERNAL="$ROOT" -j"$JOBS" "${RECONFIGURE[@]}" all 2>&1 | tee "$WORK/build.log"
"$ROOT/scripts/wsl/check-build.sh"
printf '%s\n' "$SRC_HASH" > "$STAMP"
printf '%s\n' "$CONFIG_HASH" > "$CONFIG_STAMP"
printf '%s\n' '[MRBE] Compilacao concluida. Execute: make console ou make gui'
