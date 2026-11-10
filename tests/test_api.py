"""HTTP/API guest diagnostic tests against a local deterministic mock server."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import ssl
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[1]
API_TEST = ROOT / "board/qemu-aarch64/rootfs-overlay/usr/bin/mrbe-api-test"
STABILITY_TEST = ROOT / "board/qemu-aarch64/rootfs-overlay/usr/bin/mrbe-stability-test"


class MockAPIHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/ok":
            status, content_type, body = 200, "application/json; charset=utf-8", b'{"id":1,"title":"MRBE API OK"}'
        elif self.path == "/wrong-json":
            status, content_type, body = 200, "application/json", b'{"id":2,"title":"Wrong payload"}'
        elif self.path == "/wrong-type":
            status, content_type, body = 200, "text/plain", b'{"id":1}'
        elif self.path == "/server-error":
            status, content_type, body = 503, "application/json", b'{"error":"unavailable"}'
        else:
            status, content_type, body = 404, "application/json", b'{"error":"not found"}'
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except BrokenPipeError:
            pass

    def log_message(self, *_args):
        pass


class TestAPIDiagnostics(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tempdir = tempfile.TemporaryDirectory()
        cls.cert = Path(cls.tempdir.name) / "mock-api-cert.pem"
        cls.key = Path(cls.tempdir.name) / "mock-api-key.pem"
        cert_result = subprocess.run([
            "openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
            "-keyout", str(cls.key), "-out", str(cls.cert), "-days", "2",
            "-subj", "/CN=127.0.0.1",
            "-addext", "subjectAltName=IP:127.0.0.1,DNS:localhost",
        ], capture_output=True, text=True)
        if cert_result.returncode != 0:
            raise RuntimeError(f"Could not create test TLS certificate: {cert_result.stderr}")

        cls.server = ThreadingHTTPServer(("127.0.0.1", 0), MockAPIHandler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.base_url = f"http://127.0.0.1:{cls.server.server_port}"

        cls.tls_server = ThreadingHTTPServer(("127.0.0.1", 0), MockAPIHandler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(certfile=str(cls.cert), keyfile=str(cls.key))
        cls.tls_server.socket = context.wrap_socket(cls.tls_server.socket, server_side=True)
        cls.tls_thread = threading.Thread(target=cls.tls_server.serve_forever, daemon=True)
        cls.tls_thread.start()
        cls.tls_url = f"https://127.0.0.1:{cls.tls_server.server_port}"

    @classmethod
    def tearDownClass(cls):
        for server in (cls.server, cls.tls_server):
            server.shutdown()
            server.server_close()
        cls.thread.join(timeout=2)
        cls.tls_thread.join(timeout=2)
        cls.tempdir.cleanup()

    def run_api(self, path, **overrides):
        env = os.environ.copy()
        env.update({
            "MRBE_API_URL": self.base_url + path,
            "MRBE_API_CONNECT_TIMEOUT": "2",
            "MRBE_API_TIMEOUT": "3",
            "MRBE_API_EXPECT_PATTERN": r'"id"[[:space:]]*:[[:space:]]*1',
            "MRBE_API_EXPECT_JSON": "1",
        })
        env.update(overrides)
        return subprocess.run(["sh", str(API_TEST)], env=env, text=True,
                              capture_output=True, timeout=6)

    def test_http_json_endpoint_passes(self):
        result = self.run_api("/ok")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("HTTP status: 200", result.stdout)
        self.assertIn("[PASS]", result.stdout)

    def test_api_script_runs_under_busybox_ash_when_available(self):
        busybox = shutil.which("busybox")
        if not busybox:
            self.skipTest("BusyBox is not installed on this host")
        env = os.environ.copy()
        env.update({
            "MRBE_API_URL": self.base_url + "/ok",
            "MRBE_API_EXPECT_PATTERN": r'"id"[[:space:]]*:[[:space:]]*1',
            "MRBE_API_TIMEOUT": "3",
        })
        result = subprocess.run([busybox, "ash", str(API_TEST)], env=env,
                                text=True, capture_output=True, timeout=6)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("[PASS]", result.stdout)

    def test_http_503_fails(self):
        result = self.run_api("/server-error")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Expected HTTP 200, received 503", result.stdout)

    def test_wrong_json_payload_fails(self):
        result = self.run_api("/wrong-json")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Expected response pattern", result.stdout)

    def test_wrong_content_type_fails(self):
        result = self.run_api("/wrong-type")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("not application/json", result.stdout)

    def test_https_rejects_untrusted_certificate(self):
        env = {"MRBE_API_URL": self.tls_url + "/ok", "MRBE_API_TIMEOUT": "3"}
        # Exercise the TLS server directly.
        environment = os.environ.copy()
        environment.update(env)
        tls_result = subprocess.run(["sh", str(API_TEST)], env=environment,
                                    text=True, capture_output=True, timeout=6)
        self.assertNotEqual(tls_result.returncode, 0)
        self.assertIn("Transport/DNS/TLS error", tls_result.stdout)

    def test_https_accepts_explicit_trusted_ca_without_disabling_verification(self):
        env = os.environ.copy()
        env.update({
            "MRBE_API_URL": self.tls_url + "/ok",
            "MRBE_API_TIMEOUT": "3",
            "MRBE_API_CAINFO": str(self.cert),
        })
        result = subprocess.run(["sh", str(API_TEST)], env=env, text=True,
                                capture_output=True, timeout=6)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("[PASS]", result.stdout)

    def test_invalid_api_scheme_is_rejected(self):
        result = self.run_api("/ok", MRBE_API_URL="file:///etc/passwd")
        self.assertEqual(result.returncode, 2)
        self.assertIn("must use HTTP or HTTPS", result.stdout)

    def test_missing_custom_ca_is_rejected(self):
        env = os.environ.copy()
        env.update({"MRBE_API_URL": self.tls_url + "/ok", "MRBE_API_CAINFO": "/not/a/real/ca.pem"})
        result = subprocess.run(["sh", str(API_TEST)], env=env, text=True,
                                capture_output=True, timeout=6)
        self.assertEqual(result.returncode, 2)
        self.assertIn("does not point to a readable CA", result.stdout)

    def test_stability_monitor_requires_launcher_by_default(self):
        with tempfile.TemporaryDirectory() as temp:
            env = os.environ.copy()
            env.update({
                "MRBE_STABILITY_DURATION": "0",
                "MRBE_STABILITY_INTERVAL": "1",
                "MRBE_STABILITY_MIN_MEM_KB": "1",
                "MRBE_LAUNCHER_PIDFILE": str(Path(temp) / "missing.pid"),
                "MRBE_STABILITY_LOG": str(Path(temp) / "stability.log"),
            })
            result = subprocess.run(["sh", str(STABILITY_TEST)], env=env,
                                    text=True, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 1)
            self.assertIn("launcher is not running", result.stdout)

    def test_stability_monitor_detects_low_available_memory_threshold(self):
        with tempfile.TemporaryDirectory() as temp:
            env = os.environ.copy()
            env.update({
                "MRBE_STABILITY_DURATION": "0",
                "MRBE_STABILITY_INTERVAL": "1",
                "MRBE_STABILITY_MIN_MEM_KB": "999999999999",
                "MRBE_STABILITY_REQUIRE_LAUNCHER": "0",
                "MRBE_STABILITY_LOG": str(Path(temp) / "stability.log"),
            })
            result = subprocess.run(["sh", str(STABILITY_TEST)], env=env,
                                    text=True, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 1)
            self.assertIn("below threshold", result.stdout)

    def test_stability_monitor_runs_without_launcher_when_explicitly_configured(self):
        with tempfile.TemporaryDirectory() as temp:
            log = Path(temp) / "stability.log"
            env = os.environ.copy()
            env.update({
                "MRBE_STABILITY_DURATION": "0",
                "MRBE_STABILITY_INTERVAL": "1",
                "MRBE_STABILITY_MIN_MEM_KB": "1",
                "MRBE_STABILITY_REQUIRE_LAUNCHER": "0",
                "MRBE_STABILITY_LOG": str(log),
            })
            result = subprocess.run(["sh", str(STABILITY_TEST)], env=env,
                                    text=True, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("[PASS]", result.stdout)
            self.assertTrue(log.is_file())

    def test_guest_diagnostic_scripts_are_executable_and_shell_valid(self):
        for script in (API_TEST, STABILITY_TEST,
                       ROOT / "board/qemu-aarch64/rootfs-overlay/usr/bin/mrbe-healthcheck"):
            self.assertTrue(script.is_file())
            self.assertTrue(os.access(script, os.X_OK))
            result = subprocess.run(["sh", "-n", str(script)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
