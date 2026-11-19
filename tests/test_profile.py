"""Boot profiler parsing, tested with captured guest output (no QEMU needed)."""
from pathlib import Path
import importlib.util
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
RUNNER=ROOT/'scripts/dev/boot-profile.py'
spec=importlib.util.spec_from_file_location('boot_profile',RUNNER)
bp=importlib.util.module_from_spec(spec); spec.loader.exec_module(bp)

DMESG='''[    0.099358] Memory: 932308K/1048576K available (17728K kernel code, 4912K rwdata, 12072K rodata, 3008K init, 742K bss, 79412K reserved, 32768K cma-reserved)
[    0.512000] initcall pty_init+0x0/0x364 returned 0 after 64061 usecs
[    0.520000] initcall virtio_gpu_driver_init+0x0/0x2c returned 0 after 43207 usecs
[    0.530000] initcall foo_init+0x0/0x10 returned -19 after 12 usecs
[    0.743192] Run /sbin/init as init process
'''

class TestProfileParsing(unittest.TestCase):
    def test_initcalls_sorted_slowest_first(self):
        self.assertEqual(bp.parse_initcalls(DMESG),
                         [(64061,'pty_init'),(43207,'virtio_gpu_driver_init'),(12,'foo_init')])
    def test_event_time(self):
        self.assertAlmostEqual(bp.parse_event(DMESG,'Run /sbin/init'),0.743192)
        self.assertIsNone(bp.parse_event(DMESG,'mrbe-launcher: primeira tela'))
    def test_kernel_memory(self):
        m=bp.parse_kernel_memory(DMESG)
        self.assertEqual(m['kernel'],17728)
        self.assertEqual(m['cma_reserved'],32768)
    def test_ps_skips_kernel_threads(self):
        ps='PID   RSS  COMMAND\n    1  848 init\n    2    0 [kthreadd]\n  145 8828 /usr/bin/mrbe-launcher\n'
        self.assertEqual(bp.parse_ps(ps),[(848,'init'),(8828,'/usr/bin/mrbe-launcher')])
    def test_console_gaps(self):
        lines=[(1.0,'Starting mdev... OK'),(3.2,'Starting network:'),(3.3,'OK')]
        self.assertEqual(bp.console_gaps(lines,1)[0][2],'Starting mdev... OK')
    def test_missing_images(self):
        with tempfile.TemporaryDirectory() as d:
            p=subprocess.run([sys.executable,str(RUNNER),'--root',d],capture_output=True,text=True)
            self.assertEqual(p.returncode,2)
            self.assertIn('BLOQUEADO',p.stderr)
