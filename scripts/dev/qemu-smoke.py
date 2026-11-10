#!/usr/bin/env python3
"""Boot ARM64 QEMU and require both userspace and first framebuffer render."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import select
import signal
import subprocess
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--qemu', default=os.environ.get('QEMU', 'qemu-system-aarch64'))
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=int(os.environ.get('SMOKE_TIMEOUT', '150')))
    parser.add_argument('--memory', type=int, default=int(os.environ.get('RAM_MB', '4096')))
    parser.add_argument('--smp', type=int, default=int(os.environ.get('SMP', '4')))
    parser.add_argument('--width', type=int, default=int(os.environ.get('DISPLAY_WIDTH', '1280')))
    parser.add_argument('--height', type=int, default=int(os.environ.get('DISPLAY_HEIGHT', '720')))
    args = parser.parse_args()
    gpu_device = os.environ.get('GPU_DEVICE', f'virtio-gpu-pci,xres={args.width},yres={args.height}')

    images = args.root / 'work/output/images'
    log_dir = args.root / 'logs/testes'
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / 'qemu-boot-latest.log'
    report_path = log_dir / 'qemu-smoke-latest.json'
    kernel, rootfs = images / 'Image', images / 'rootfs.ext4'
    for artifact in (kernel, rootfs):
        if not artifact.is_file() or artifact.stat().st_size == 0:
            print(f'[BLOQUEADO] Imagem ausente: {artifact}. Rode: make build', file=sys.stderr)
            return 2
    if args.timeout <= 0 or args.memory < 256 or args.smp < 1 or args.width < 640 or args.height < 360:
        print('[ERRO] Timeout/memoria/CPU invalidos', file=sys.stderr)
        return 2

    cmd = [args.qemu, '-M', 'virt', '-cpu', 'cortex-a53', '-smp', str(args.smp),
           '-m', str(args.memory), '-accel', 'tcg', '-kernel', str(kernel),
           '-append', f'root=/dev/vda rootwait console=ttyAMA0 rw loglevel=4 video=Virtual-1:{args.width}x{args.height}@60',
           '-drive', f'file={rootfs},if=none,format=raw,id=hd0,snapshot=on',
           '-device', 'virtio-blk-device,drive=hd0',
           '-netdev', 'user,id=net0', '-device', 'virtio-net-device,netdev=net0',
           '-device', gpu_device, '-device', 'virtio-keyboard-pci', '-device', 'virtio-mouse-pci', '-display', 'none', '-serial', 'stdio', '-monitor', 'none']

    started = time.monotonic()
    proc = None
    boot_marker = b'MRBE_BOOT_OK'
    ui_marker = b'MRBE_UI_READY'
    assets_marker = b'MRBE_HOME_ASSETS_READY'
    input_marker = b'MRBE_INPUT_READY'
    api_tools_marker = b'MRBE_API_TOOLS_READY'
    tail = b''
    found_boot = False
    found_ui = False
    found_assets = False
    found_input = False
    found_api_tools = False
    result = 'FAIL'
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                start_new_session=True, bufsize=0)
        assert proc.stdout is not None
        with log_path.open('wb') as log:
            while time.monotonic() - started < args.timeout:
                if proc.poll() is not None:
                    extra = proc.stdout.read() or b''
                    log.write(extra)
                    tail = (tail + extra)[-16384:]
                    found_boot = found_boot or boot_marker in tail
                    found_ui = found_ui or ui_marker in tail
                    found_assets = found_assets or assets_marker in tail
                    found_input = found_input or input_marker in tail
                    found_api_tools = found_api_tools or api_tools_marker in tail
                    break
                readable, _, _ = select.select([proc.stdout], [], [], 0.3)
                if not readable:
                    continue
                chunk = os.read(proc.stdout.fileno(), 8192)
                if not chunk:
                    continue
                log.write(chunk)
                log.flush()
                tail = (tail + chunk)[-16384:]
                found_boot = found_boot or boot_marker in tail
                found_ui = found_ui or ui_marker in tail
                found_assets = found_assets or assets_marker in tail
                found_input = found_input or input_marker in tail
                found_api_tools = found_api_tools or api_tools_marker in tail
                if found_boot and found_ui and found_assets and found_input and found_api_tools:
                    break
        result = 'PASS' if found_boot and found_ui and found_assets and found_input and found_api_tools else 'FAIL'
    except (FileNotFoundError, PermissionError) as exc:
        result = 'ERROR'
        print(f'[ERRO] QEMU indisponivel: {exc}', file=sys.stderr)
    finally:
        if proc is not None and proc.poll() is None:
            try:
                os.killpg(proc.pid, signal.SIGTERM)
                proc.wait(timeout=5)
            except (ProcessLookupError, subprocess.TimeoutExpired):
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                proc.wait()
        report = {
            'result': result,
            'boot_marker': 'MRBE_BOOT_OK',
            'ui_marker': 'MRBE_UI_READY',
            'assets_marker': 'MRBE_HOME_ASSETS_READY',
            'input_marker': 'MRBE_INPUT_READY',
            'api_tools_marker': 'MRBE_API_TOOLS_READY',
            'boot_marker_found': found_boot,
            'ui_marker_found': found_ui,
            'assets_marker_found': found_assets,
            'input_marker_found': found_input,
            'api_tools_marker_found': found_api_tools,
            'qemu_memory_mib': args.memory,
            'qemu_smp': args.smp,
            'qemu_display_width': args.width,
            'qemu_display_height': args.height,
            'elapsed_sec': round(time.monotonic() - started, 2),
            'log': str(log_path),
            'kernel': str(kernel),
            'rootfs': str(rootfs),
        }
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    if found_boot and found_ui and found_assets and found_input and found_api_tools:
        print(f'[APROVADO] Boot, assets, interface, entrada e ferramentas API reconhecidos; {report["elapsed_sec"]}s. Log: {log_path}')
        return 0
    print(f'[FALHA] Boot/UI/API tools nao comprovados (boot={found_boot}, assets={found_assets}, ui={found_ui}, input={found_input}, api_tools={found_api_tools}). Relatorio: {report_path}',
          file=sys.stderr)
    if tail:
        print(tail[-3000:].decode('utf-8', errors='replace'), file=sys.stderr)
    return 1


if __name__ == '__main__':
    sys.exit(main())
