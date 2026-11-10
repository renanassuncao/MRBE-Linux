from pathlib import Path
import json
import unittest
ROOT=Path(__file__).resolve().parents[1]
class TestProject(unittest.TestCase):
    def test_tree(self):
        for f in ('VERSION','external.desc','external.mk','Config.in',
                  'configs/mrbe_qemu_aarch64_defconfig',
                  'board/qemu-aarch64/linux.fragment',
                  'board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe'):
            self.assertTrue((ROOT/f).is_file(),f)
    def test_config(self):
        c=(ROOT/'configs/mrbe_qemu_aarch64_defconfig').read_text()
        self.assertIn('BR2_aarch64=y',c)
        self.assertIn('BR2_TOOLCHAIN_BUILDROOT_MUSL=y',c)
        self.assertIn('BR2_LINUX_KERNEL_USE_ARCH_DEFAULT_CONFIG=y',c)
        self.assertIn('BR2_PACKAGE_LIBCURL_CURL=y',c)
        self.assertIn('BR2_PACKAGE_LIBCURL_OPENSSL=y',c)
        self.assertIn('BR2_PACKAGE_CA_CERTIFICATES=y',c)
        self.assertNotIn('board/qemu/aarch64-virt/linux.config',c)
        self.assertNotIn('BR2_GLOBAL_PATCH_DIR="board/qemu/patches"',c)
    def test_build_script_merges_api_options_into_existing_config(self):
        build = (ROOT/'scripts/wsl/build.sh').read_text()
        helper = ROOT/'scripts/wsl/ensure-api-config.sh'
        self.assertIn('ensure-api-config.sh', build)
        with __import__('tempfile').TemporaryDirectory() as temp:
            config = Path(temp)/'.config'
            config.write_text('BR2_PACKAGE_CURL=n\n# BR2_PACKAGE_CA_CERTIFICATES is not set\nCUSTOM_OPTION=y\n')
            result = __import__('subprocess').run(['bash', str(helper), str(config)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            merged = config.read_text()
            for option in ('BR2_PACKAGE_OPENSSL=y','BR2_PACKAGE_LIBCURL=y','BR2_PACKAGE_LIBCURL_OPENSSL=y','BR2_PACKAGE_LIBCURL_CURL=y','BR2_PACKAGE_CA_CERTIFICATES=y'):
                self.assertIn(option, merged)
            self.assertIn('CUSTOM_OPTION=y', merged)
            self.assertNotIn('BR2_PACKAGE_CURL=', merged)
            self.assertNotIn('# BR2_PACKAGE_CA_CERTIFICATES is not set', merged)

    def test_boot_marker(self):
        s=(ROOT/'board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe').read_text()
        self.assertIn('MRBE_BOOT_OK',s)
        self.assertIn('MRBE_API_TOOLS_READY',s)
    def test_status(self):
        s=json.loads((ROOT/'status.json').read_text())
        self.assertEqual(s['version'],'0.7.0')
        self.assertEqual(s['qemu_boot'],'pending_user_WSL_build_QEMU_smoke_and_guest_api')
        self.assertIn('33_tests',s['host_test_result'])
        self.assertEqual(s['visual_identity'],'0.6.0_frozen_no_visual_changes_in_stability_release')
    def test_no_hardware_flash(self):
        self.assertFalse((ROOT/'images/mxq-pro-flash.img').exists())

    def test_ui_autostart_and_smoke_contract(self):
        init = (ROOT/'board/qemu-aarch64/rootfs-overlay/etc/init.d/S99mrbe').read_text()
        main = (ROOT/'package/mrbe-launcher/src/main.c').read_text()
        smoke = (ROOT/'scripts/dev/qemu-smoke.py').read_text()
        self.assertIn('/usr/bin/mrbe-launcher', init)
        self.assertIn('MRBE_UI_READY', main)
        self.assertIn('MRBE_HOME_ASSETS_READY', main)
        self.assertIn('MRBE_INPUT_READY', main)
        self.assertIn('MRBE_API_TOOLS_READY', init)
        self.assertIn('MRBE_UI_READY', smoke)
        self.assertIn('MRBE_HOME_ASSETS_READY', smoke)
        self.assertIn('MRBE_INPUT_READY', smoke)
        self.assertIn('MRBE_API_TOOLS_READY', smoke)
        self.assertIn('DISPLAY_WIDTH', smoke)
        self.assertIn('video=Virtual-1', smoke)
        gui = (ROOT/'scripts/wsl/run-qemu-gui.sh').read_text()
        self.assertIn('DISPLAY_WIDTH', gui)
        self.assertIn('DISPLAY_HEIGHT', gui)
        self.assertIn('virtio-mouse-pci', gui)
        check = (ROOT/'scripts/wsl/check-build.sh').read_text()
        self.assertIn('/usr/share/mrbe/ui', check)
        self.assertIn('31', check)
        self.assertIn('mrbe-api-test', check)
        self.assertIn('ca-certificates.crt', check)

    def test_brand_asset(self):
        asset = ROOT/'package/mrbe-launcher/src/assets/brasao-256.rgba'
        self.assertTrue(asset.is_file())
        self.assertEqual(asset.stat().st_size, 256*256*4)


    def test_home_visual_assets(self):
        assets = ROOT/'package/mrbe-launcher/src/assets'
        expected = {
            'home-hero.rgba': 1120*340*4,
            'brand-lockup.rgba': 320*230*4,
        }
        for i in range(5):
            expected[f'home-card-{i}.rgba'] = 400*250*4
        for i in range(9):
            expected[f'app-tile-{i}.rgba'] = 360*220*4
        for i in range(7):
            expected[f'nav-label-{i}.rgba'] = 160*40*4
            expected[f'nav-{["home","guide","apps","store","search","library","settings"][i]}.rgba'] = 128*128*4
        for name, size in expected.items():
            asset = assets/name
            self.assertTrue(asset.is_file(), name)
            self.assertEqual(asset.stat().st_size, size, name)
        mk = (ROOT/'package/mrbe-launcher/mrbe-launcher.mk').read_text()
        self.assertIn('/usr/share/mrbe/ui', mk)
        self.assertIn('assets/*.rgba', mk)

    def test_brand_asset_installed(self):
        mk = (ROOT/'package/mrbe-launcher/mrbe-launcher.mk').read_text()
        self.assertIn('MRBE_LAUNCHER_VERSION = 0.7.0', mk)
        self.assertIn('brasao-256.rgba', mk)
        self.assertIn('/usr/share/mrbe/brand/', mk)

if __name__=='__main__': unittest.main()
