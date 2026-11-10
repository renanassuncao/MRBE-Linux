.PHONY: help setup build ui-rebuild check console gui smoke preview test api-test clean
help:
	@printf '%s\n' 'MRBE TV OS 0.7.0' 'make setup   - instalar ferramentas Linux/WSL' 'make test    - testes locais sem Buildroot' 'make api-test - testes HTTP/JSON com API simulada local' 'make preview - renderizar interface sem QEMU' 'make build   - compilar imagem ARM64' 'make ui-rebuild - recompilar launcher e imagem para QEMU' 'make check   - checar artefatos gerados' 'make console - executar QEMU via serial' 'make gui     - executar QEMU 4 GiB com tela' 'make smoke   - smoke test boot QEMU (requer build)' 'make clean   - remover apenas work/output (com confirmacao manual)'
setup:
	./scripts/wsl/install-dev-tools.sh
build:
	./scripts/wsl/build.sh
ui-rebuild:
	MRBE_FORCE_LAUNCHER_REBUILD=1 ./scripts/wsl/build.sh
check:
	./scripts/wsl/check-build.sh
console:
	./scripts/wsl/run-qemu-console.sh
gui:
	./scripts/wsl/run-qemu-gui.sh
smoke:
	./scripts/wsl/run-qemu-smoke.sh
preview:
	./scripts/dev/render-preview.sh
test:
	./scripts/dev/test-local.sh
api-test:
	python3 -m unittest discover -s tests -p 'test_api.py' -v
clean:
	@echo 'Para limpeza, execute conscientemente: rm -rf work/output'
