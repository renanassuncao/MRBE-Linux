from pathlib import Path
import json
import unittest
ROOT=Path(__file__).resolve().parents[1]
class TestProject(unittest.TestCase):
    def test_tree(self):
        for f in ('VERSION','external.desc','external.mk','Config.in',
                  'configs/mrbe_qemu_aarch64_defconfig',
                  'board/qemu-aarch64/linux.fragment',
                  'board/qemu-aarch64/rootfs-overlay/etc/init.d/S05mrbe-launcher',
                  'board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe'):
            self.assertTrue((ROOT/f).is_file(),f)
    def test_config(self):
        c=(ROOT/'configs/mrbe_qemu_aarch64_defconfig').read_text()
        self.assertIn('BR2_aarch64=y',c)
        self.assertIn('BR2_TOOLCHAIN_BUILDROOT_MUSL=y',c)
        self.assertIn('BR2_LINUX_KERNEL_USE_ARCH_DEFAULT_CONFIG=y',c)
        self.assertNotIn('board/qemu/aarch64-virt/linux.config',c)
        self.assertNotIn('BR2_GLOBAL_PATCH_DIR="board/qemu/patches"',c)
    def test_boot_marker(self):
        s=(ROOT/'board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe').read_text()
        self.assertIn('MRBE_BOOT_OK',s)
    def test_launcher_starts_before_network(self):
        # Sem cabo, o DHCP de S40network segura o boot; a tela nao deve esperar.
        init=ROOT/'board/qemu-aarch64/rootfs-overlay/etc/init.d'
        launcher=[f.name for f in init.iterdir() if 'exec /usr/bin/mrbe-launcher' in f.read_text()]
        self.assertEqual(launcher,['S05mrbe-launcher'])
        self.assertLess(int(launcher[0][1:3]),40)
    def test_status(self):
        s=json.loads((ROOT/'status.json').read_text())
        self.assertEqual(s['version'],'0.4.0')
        self.assertEqual(s['qemu_boot'],'0.2_reported_working_by_user; 0.4_pending')
        self.assertEqual(s['phases']['branding'],'0.4_implemented_source_and_assets')
    def test_no_hardware_flash(self):
        self.assertFalse((ROOT/'images/mxq-pro-flash.img').exists())

    def test_brand_asset(self):
        asset = ROOT/'package/mrbe-launcher/src/assets/brasao-256.rgba'
        self.assertTrue(asset.is_file())
        self.assertEqual(asset.stat().st_size, 256*256*4)

    def test_brand_asset_installed(self):
        mk = (ROOT/'package/mrbe-launcher/mrbe-launcher.mk').read_text()
        self.assertIn('MRBE_LAUNCHER_VERSION = 0.4.0', mk)
        self.assertIn('brasao-256.rgba', mk)
        self.assertIn('/usr/share/mrbe/brand/', mk)

if __name__=='__main__': unittest.main()
