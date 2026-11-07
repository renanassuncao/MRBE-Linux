# 06 — Primeiro boot e controles

## O que esperar

No modo serial:
- mensagens do firmware não são necessárias porque o kernel é carregado diretamente;
- mensagens do kernel;
- montagem do root filesystem;
- BusyBox init;
- login root no `ttyAMA0`.

No modo gráfico:
- QEMU abre uma janela;
- VirtIO GPU deve criar o dispositivo gráfico;
- o init tenta iniciar `/usr/bin/mrbe-launcher` se `/dev/fb0` existir;
- o launcher desenha quatro cartões.

Enquanto o launcher está na tela, o terminal virtual fica em modo gráfico: o
console do kernel não desenha cursor nem mensagens por cima da interface. Ao
parar o launcher, o console de texto volta. A
linha `mrbe-launcher: primeira tela` no `dmesg` marca quando a interface
apareceu, no mesmo relógio das mensagens de boot.

## Controles do launcher

| Tecla | Ação |
|---|---|
| ← / ↑ | Cartão anterior |
| → / ↓ | Próximo cartão |
| Enter | Abrir informação do cartão |
| Esc / Backspace | Voltar à tela inicial |

## Cartões demonstrativos

- Media: informa que o player ainda não faz parte do protótipo.
- Apps: informa que o catálogo será desenvolvido.
- Network: lê o estado de `eth0`.
- System: lê memória total e disponível de `/proc/meminfo`.

## Se a janela ficar preta

No console serial, execute:

```sh
mrbe-diagnostico
cat /var/log/mrbe-launcher.log
```

Procure por `/dev/fb0`. Se não existir, a falha está no caminho gráfico do kernel/driver/console, não necessariamente no Buildroot inteiro.
