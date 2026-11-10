# MRBE TV OS 0.6.0 — Home cinematográfica

## Objetivo
Fazer a Home aprovada aparecer no QEMU como interface real do launcher C, sem X11, Wayland ou navegador.

## Composição visual
- Canvas lógico: 1280x720, escala proporcional para outros framebuffers.
- Sidebar: 160 px, com sete destinos e ícones/labels próprios.
- Hero: 1120x340, imagem cinematográfica com ações na arte de referência.
- Recomendações: painel com cinco cartões RGBA.
- Apps: nove tiles RGBA com foco selecionável.
- Status: relógio do sistema, rede e perfil desenhados pelo launcher.

## Assets e dimensões
Os arquivos ficam em `package/mrbe-launcher/src/assets/` e são instalados em `/usr/share/mrbe/ui/`.

| Tipo | Quantidade | Dimensão |
|---|---:|---:|
| Hero | 1 | 1120x340 |
| Cartões de recomendação | 5 | 400x250 |
| Tiles de aplicativos | 9 | 360x220 |
| Ícones de navegação | 7 | 128x128 |
| Rótulos de navegação | 7 | 160x40 |
| Brand lockup | 1 | 320x230 |

Os arquivos `.rgba` são RGBA8888 bruto, sem cabeçalho. As dimensões são definidas na chamada `mrbe_fb_load_rgba()`.

## Renderização
O launcher continua usando Linux fbdev. A função `mrbe_fb_draw_rgba()` faz blit nearest-neighbour com clipping e conversão para o formato do framebuffer. Pixels opacos são tratados sem passar por uma chamada de desenho por pixel, reduzindo o custo de renderização da arte.

O caminho principal dos assets em runtime é `/usr/share/mrbe/ui/`. O preview host usa fallback para `package/mrbe-launcher/src/assets/`.

## Navegação
- Sidebar: Home, Guia, Apps, Loja, Pesquisar, Biblioteca e Config.
- Setas/Enter/Esc: navegação por controle remoto/teclado.
- Mouse relativo: cursor, clique esquerdo para selecionar, botão direito para voltar e roda para navegar.
- Home: foco sequencial Hero → recomendações → aplicativos.
- Tile “Todos os apps” abre a tela Apps.

## Testes host
```bash
make test
```
O teste verifica sintaxe, compilação C com warnings como erros, state machine, mouse, contrato de assets, smoke runner e prévias.

## Rebuild e teste no WSL
Não apagar `work/`. A saída Buildroot existente deve ser reutilizada.

```bash
cd ~/MRBE-Linux
make test
JOBS=2 make ui-rebuild
make check
make smoke
make gui
```

A janela usa 4 GiB e solicita 1280x720. Para 1080p:
```bash
DISPLAY_WIDTH=1920 DISPLAY_HEIGHT=1080 make gui
```

O smoke test deve encontrar:
- `MRBE_BOOT_OK`
- `MRBE_HOME_ASSETS_READY`
- `MRBE_UI_READY`
- `MRBE_INPUT_READY`

Se `MRBE_HOME_ASSETS_READY` não aparecer, verifique se os `.rgba` foram instalados no rootfs:
```bash
find work/output/target/usr/share/mrbe/ui -maxdepth 1 -name '*.rgba' -printf '%f %s bytes\\n'
```

## Limites
Os cartões de Netflix, Disney+, Prime Video e Max são atalhos visuais demonstrativos. Não há instalação, DRM/Widevine, certificação ou reprodução de streaming comercial. O QEMU `virt` AArch64 não é uma imagem para gravar no S905X3.
