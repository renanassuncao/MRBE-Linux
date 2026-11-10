# 22 — Interface MRBE TV OS 0.5.0 e validação

## Objetivo
Launcher gráfico leve, diretamente no framebuffer Linux, com inicialização automática no QEMU e navegação direcional.

## Telas
1. Home: destaque MRBE, quatro streamings prioritários e recomendações ilustrativas.
2. Apps: catálogo e filtros Todos/Favoritos/Recentes.
3. MRBE Store: Destaques, Streaming, Mídia e Utilitários.
4. Detalhes: descrição, plataforma, compatibilidade e favoritos.
5. Biblioteca: Todos, Favoritos e Recentes, sem duplicar entradas.
6. Filmes, Séries, Esportes e Infantil: cartões ilustrativos sem reprodução.
7. Pesquisa: atalhos por categoria; pesquisa textual pendente.
8. Configurações: rede, imagem, som, controle remoto, aplicativos, armazenamento, sistema e sobre.
9. Painel rápido: sobreposição demonstrativa.

## Controles
| Tecla | Ação |
|---|---|
| Setas | Navegar e mover o foco |
| Enter | Selecionar |
| Esc / Backspace | Voltar |
| H | Home |
| A | Apps |
| S | Configurações |
| Q | Pesquisa |
| M | Painel rápido |
| F | Favorito |

O launcher consome eventos de teclado/controle direcional via evdev. Embora o QEMU exponha um dispositivo virtio-mouse, o ponteiro do mouse ainda não é desenhado nem usado como mecanismo de seleção.

## Inicialização automática e prova de UI
`S99mrbe` aguarda `/dev/fb0` por até 30 segundos. Quando disponível, inicia `/usr/bin/mrbe-launcher`. Depois de desenhar o primeiro quadro, o launcher escreve `MRBE_UI_READY` em `/dev/console`.

`make smoke` só passa quando o console serial contém:
- `MRBE_BOOT_OK`: userspace iniciado.
- `MRBE_UI_READY`: launcher abriu o framebuffer e desenhou a primeira tela.
- `MRBE_INPUT_READY`: ao menos um dispositivo evdev de teclado/controle foi aberto.

Isso não substitui a inspeção visual com `make gui`.

## Comandos WSL2
```bash
make test
make preview  # gera as 11 telas; make test usa uma previa rapida de 4 telas
JOBS=2 make ui-rebuild
make check
make smoke
make gui
```

QEMU com 1 GiB para comparação:
```bash
RAM_MB=1024 make gui
```

## Limites
Os cartões Netflix, Disney+, Prime Video e Max são demonstrativos. O botão Abrir/Instalar informa que a plataforma oficial, DRM e certificações são requisitos externos. Não há contorno de DRM, instalação de APK nem streaming no QEMU Linux.
