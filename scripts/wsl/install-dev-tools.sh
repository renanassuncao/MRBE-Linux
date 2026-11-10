#!/usr/bin/env bash
set -euo pipefail

echo "=== MRBE Linux: preparando Ubuntu/WSL ==="
sudo apt-get update
sudo apt-get install -y \
  build-essential gcc g++ make bc bison flex \
  file git wget curl rsync cpio unzip xz-utils \
  patch perl python3 python3-venv \
  libncurses-dev libssl-dev openssl libelf-dev zlib1g-dev \
  device-tree-compiler \
  qemu-system-arm qemu-system-gui qemu-utils

echo
echo "=== Verificações ==="
command -v make
command -v qemu-system-aarch64
qemu-system-aarch64 --version | head -n 1
python3 --version
echo
echo "Dependências instaladas. Próximo passo: ./scripts/wsl/build.sh"
