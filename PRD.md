# PRD — MRBE TV OS 0.7.0

## Visão
Criar uma plataforma Linux embarcada de TV, aberta e leve, com identidade visual MRBE própria, operável por controle remoto direcional e mouse, e build reproduzível em Windows + WSL2.

## Alvo atual de desenvolvimento
- QEMU `virt` AArch64.
- 4 GiB de RAM no QEMU GUI por padrão.
- Resolução solicitada de 1280x720; teste opcional em 1920x1080.
- Buildroot 2025.02.19 + musl + BusyBox.
- Kernel Linux 6.12.27.
- Launcher C no framebuffer, sem X11/Wayland/desktop pesado.

## Escopo visual congelado 0.6.0
1. Home MRBE cinematográfica baseada na arte de referência aprovada, sem copiar a interface proprietária de outro fabricante.
2. Hero de conteúdo, recomendações em cinco cartões e nove tiles de aplicativos.
3. Barra lateral de sete destinos: Home, Guia, Apps, Loja, Pesquisar, Biblioteca e Configurações.
4. Relógio, indicador de rede e perfil desenhados na camada do launcher.
5. Assets RGBA instalados em `/usr/share/mrbe/ui/` no rootfs.
6. Navegação por setas, Enter, Esc, H, A, S, Q, M e F; mouse relativo, clique esquerdo, botão direito e roda.
7. Smoke test exige `MRBE_BOOT_OK`, `MRBE_HOME_ASSETS_READY`, `MRBE_UI_READY` e `MRBE_INPUT_READY`.
8. Pré-visualização host das telas e testes automatizados sem exigir boot QEMU.
9. QEMU GUI com 4 GiB e solicitação de 1280x720.

## Escopo 0.7.0 — estabilidade e API
1. Preservar a interface visual 0.6.0 sem alteração de layout.
2. Incluir curl/libcurl, TLS OpenSSL e certificados CA no rootfs.
3. Disponibilizar `mrbe-healthcheck`, `mrbe-api-test` e `mrbe-stability-test`.
4. Testar HTTP 200/503, payload JSON, Content-Type, TLS confiável e TLS não confiável com servidor mock local.
5. Monitorar launcher, MemAvailable e RSS por 5 minutos no guest.
6. Smoke test exige a presença dos utilitários API e do bundle de certificados.
7. Não gravar tokens/chaves de produção em código ou logs.

## Fora do escopo desta versão
- Instalação de APKs, Google Play, Netflix/Disney+/Prime Video/Max oficiais no Linux, Widevine/DRM ou reprodução de streaming comercial.
- Pesquisa textual completa, instalação real de pacotes, áudio, OTA, controle de energia e controle parental funcional.
- Suporte físico ao Amlogic S905X3 sem identificar PCB, DTB, bootloader, BSP e método de recuperação.

## Critérios de aceite
A1: `make test` passa (sintaxe, compilação host, testes Python e prévias).
A2: `JOBS=2 make ui-rebuild` recompila launcher e imagem ARM64 no WSL.
A3: `make smoke` encontra os marcadores de boot, assets, UI, evdev e ferramentas API/CA.
A4: `make gui` abre a Home cinematográfica em 1280x720.
A5: Home, menu lateral, recomendações e aplicativos respondem a teclado/controle remoto e mouse.
A6: logs e evidências são arquivados em `logs/testes/`.

Nenhum critério de boot QEMU 0.7.0, comunicação externa de API ou estabilidade no guest será marcado como concluído antes da execução no ambiente do usuário.
