# Roteiro de validação MRBE TV OS 0.5.0

## 1. Testes locais
```bash
make test
```
Esperado: sintaxe shell/C, testes de estado, prévias de todas as telas.

## 2. Reconstruir launcher e imagem
```bash
JOBS=2 make ui-rebuild
make check
```
Esperado: `work/output/images/Image` e `rootfs.ext4` atualizados.

## 3. Boot sem interface
```bash
make smoke
```
Esperado: marcador `MRBE_BOOT_OK`. Isto testa boot/init, não a interface.

## 4. Interface gráfica
```bash
make gui
```
Esperado: janela QEMU com Home MRBE TV OS 0.5.0.

Controles:
- Setas / Enter / Esc.
- H, A, S, Q, M, F.

## 5. Se a tela estiver preta ou ficar no terminal
No console serial do guest:
```sh
ls -l /dev/fb0
cat /var/log/mrbe-launcher.log
ps | grep mrbe-launcher
```

## 6. Perfil 1 GiB para hosts limitados
```bash
RAM_MB=1024 SMP=2 make gui
RAM_MB=1024 SMP=2 make smoke
```

## 7. Evidências a enviar
- Saída integral de `make test`.
- Final de `work/build.log`.
- Saída de `make check`.
- Saída de `make smoke`.
- Captura da janela `make gui`.
- Se falhar: `/var/log/mrbe-launcher.log` e `dmesg | grep -Ei 'virtio|drm|fb'`.
