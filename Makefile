.PHONY: help setup build ui-rebuild check console gui smoke profile preview test clean
help:
	@printf '%s\n' 'MRBE Linux TV 0.4.0' 'make setup   - instalar ferramentas Linux/WSL' 'make test    - testes locais sem Buildroot' 'make preview - renderizar interface sem QEMU' 'make build   - compilar imagem ARM64' 'make ui-rebuild - recompilar launcher e imagem para QEMU' 'make check   - checar artefatos gerados' 'make console - executar QEMU via serial' 'make gui     - executar QEMU com tela' 'make smoke   - smoke test boot QEMU (requer build)' 'make profile - perfil de boot: kernel, init e memoria (requer build)' 'make clean   - remover apenas work/output (com confirmacao manual)' 
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
profile:
	./scripts/wsl/run-qemu-profile.sh
preview:
	./scripts/dev/render-preview.sh
test:
	./scripts/dev/test-local.sh
clean:
	@echo 'Para limpeza, execute conscientemente: rm -rf work/output'
