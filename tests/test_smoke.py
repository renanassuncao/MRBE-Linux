"""Smoke runner behavior tested using a fake QEMU, not a Linux guest."""
from pathlib import Path
import json
import shlex
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
RUNNER=ROOT/'scripts/dev/qemu-smoke.py'

class TestSmokeRunner(unittest.TestCase):
    def test_missing_images(self):
        with tempfile.TemporaryDirectory() as d:
            p=subprocess.run([sys.executable,str(RUNNER),'--root',d,'--timeout','2'],
                             capture_output=True,text=True)
            self.assertEqual(p.returncode,2)
            self.assertIn('BLOQUEADO',p.stderr)
    def test_fake_boot_and_ui_markers(self):
        self._exercise('MRBE_BOOT_OK\nMRBE_HOME_ASSETS_READY\nMRBE_UI_READY\nMRBE_INPUT_READY\nMRBE_API_TOOLS_READY',0,'PASS')
    def test_fake_boot_without_ui_marker(self):
        self._exercise('MRBE_BOOT_OK',1,'FAIL')
    def test_fake_boot_without_assets_marker(self):
        self._exercise('MRBE_BOOT_OK\nMRBE_UI_READY\nMRBE_INPUT_READY',1,'FAIL')
    def test_fake_boot_without_input_marker(self):
        self._exercise('MRBE_BOOT_OK\nMRBE_UI_READY',1,'FAIL')
    def test_fake_boot_without_api_tools_marker(self):
        self._exercise('MRBE_BOOT_OK\nMRBE_HOME_ASSETS_READY\nMRBE_UI_READY\nMRBE_INPUT_READY',1,'FAIL')
    def test_fake_boot_without_marker(self):
        self._exercise('OTHER_MESSAGE',1,'FAIL')
    def _exercise(self, output, expected_rc, expected_status):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            images=base/'work/output/images'; images.mkdir(parents=True)
            for n in ('Image','rootfs.ext4'):
                (images/n).write_bytes(b'placeholder (not a real boot image)')
            fake=base/'fake-qemu'
            fake.write_text('#!/bin/sh\nprintf %s '+shlex.quote(output)+'\nexit 0\n')
            fake.chmod(0o755)
            r=subprocess.run([sys.executable,str(RUNNER),'--root',d,'--qemu',str(fake),
                              '--timeout','3'],capture_output=True,text=True)
            self.assertEqual(r.returncode,expected_rc,r.stderr)
            report=json.loads((base/'logs/testes/qemu-smoke-latest.json').read_text())
            self.assertEqual(report['result'],expected_status)
            self.assertEqual(report['qemu_memory_mib'], 4096)
            self.assertEqual(report['qemu_smp'], 4)
            self.assertEqual(report['qemu_display_width'], 1280)
            self.assertEqual(report['qemu_display_height'], 720)
            self.assertIn(output,(base/'logs/testes/qemu-boot-latest.log').read_text())
