# MRBE TV OS 0.5.1 — resolução, mouse e foco

## Objetivo
Corrigir a apresentação 4:3 observada na captura do QEMU, preservar a proporção 16:9, habilitar ponteiro/click de mouse e reduzir o corte da barra inferior.

## Alterações
- QEMU solicita modo 1280x720 com `xres`/`yres` no virtio-gpu e `video=Virtual-1:1280x720@60`.
- `DISPLAY_WIDTH` e `DISPLAY_HEIGHT` podem sobrescrever a resolução.
- O renderer usa escala uniforme e letterboxing para framebuffers fora de 16:9.
- evdev agrega teclas, movimento relativo do mouse, BTN_LEFT, BTN_RIGHT e REL_WHEEL/REL_HWHEEL.
- O ponteiro é desenhado no framebuffer e usa hit-testing por tela.
- A barra inferior foi encurtada para caber em resoluções menores.

## Comandos no WSL
```bash
cd ~/MRBE-Linux
make test
JOBS=2 make ui-rebuild
make check
make smoke
make gui
```

Para testar 720p explicitamente:
```bash
DISPLAY_WIDTH=1280 DISPLAY_HEIGHT=720 make gui
```

Para comparar 1080p:
```bash
DISPLAY_WIDTH=1920 DISPLAY_HEIGHT=1080 make gui
```

Se a versão do QEMU não reconhecer `xres`/`yres`:
```bash
GPU_DEVICE=virtio-gpu-pci make gui
GPU_DEVICE=virtio-gpu-pci make smoke
```
O parâmetro `video=Virtual-1` continua solicitando o modo ao driver Linux.

## Controles
- Setas / Enter / Esc: controle remoto/teclado.
- Mouse: mover exibe ponteiro.
- Clique esquerdo: selecionar.
- Clique direito: voltar.
- Roda vertical: navegar para cima/baixo.
- Roda horizontal: navegar esquerda/direita.

## Critérios de aceite
1. `make test` passa.
2. `make ui-rebuild` compila o launcher e imagem.
3. `make smoke` encontra boot, UI e input.
4. `make gui` informa framebuffer próximo de 1280x720.
5. Mouse move cursor e seleciona itens.
6. Fallback 4:3 mostra letterbox, sem deformar o layout.
7. Rodapé não ultrapassa a borda da tela.

## Limites
- A resolução solicitada pode não ser aceita por versões antigas de QEMU/driver. Conferir a linha `MRBE framebuffer: WIDTHxHEIGHT` no console.
- Mouse relativo funciona com dispositivos evdev que emitem `REL_X`, `REL_Y` e `BTN_LEFT`.
- As telas de streaming continuam demonstrativas; este release não implementa DRM nem aplicativos comerciais.
- Teste QEMU real deve ser executado no WSL do usuário; testes host não substituem essa validação.
