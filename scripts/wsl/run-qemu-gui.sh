#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/work/output/images"
QEMU="${QEMU:-qemu-system-aarch64}"
RAM_MB="${RAM_MB:-4096}"
SMP="${SMP:-4}"
DISPLAY_WIDTH="${DISPLAY_WIDTH:-1280}"
DISPLAY_HEIGHT="${DISPLAY_HEIGHT:-720}"
GPU_DEVICE="${GPU_DEVICE:-virtio-gpu-pci,xres=${DISPLAY_WIDTH},yres=${DISPLAY_HEIGHT}}"
for f in "$OUT/Image" "$OUT/rootfs.ext4"; do
    [ -s "$f" ] || { echo "[ERRO] $f ausente; execute make build"; exit 2; }
done
command -v "$QEMU" >/dev/null || { echo 'Instale qemu-system-arm'; exit 2; }
if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    echo '[ERRO] Sem WSLg ou servidor grafico. No Windows 11: wsl --update, reinicie o WSL.'
    echo 'Enquanto isso use make console; ou make preview no Linux.'
    exit 2
fi
echo "MRBE TV OS 0.7.0: QEMU ARM64; RAM ${RAM_MB} MiB; CPUs ${SMP}; tela ${DISPLAY_WIDTH}x${DISPLAY_HEIGHT}; teclado e mouse"
echo "Para limitar memoria: RAM_MB=1024 make gui"
exec "$QEMU" \
    -M virt -cpu cortex-a53 -smp "$SMP" -m "$RAM_MB" -accel tcg \
    -kernel "$OUT/Image" \
    -append "root=/dev/vda rootwait console=ttyAMA0 rw loglevel=4 video=Virtual-1:${DISPLAY_WIDTH}x${DISPLAY_HEIGHT}@60" \
    -drive "file=$OUT/rootfs.ext4,if=none,format=raw,id=hd0,snapshot=on" \
    -device virtio-blk-device,drive=hd0 \
    -netdev user,id=net0 -device virtio-net-device,netdev=net0 \
    -device "$GPU_DEVICE" -device virtio-keyboard-pci -device virtio-mouse-pci \
    -display gtk,gl=off -serial stdio -monitor none
