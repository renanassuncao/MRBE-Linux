# Changelog

## 0.7.0 — estabilidade e diagnósticos de API
- Mantida a identidade visual 0.6.0 sem alterações de layout.
- Habilitado curl com TLS OpenSSL e bundle de certificados CA no Buildroot.
- Adicionado `mrbe-api-test`: verifica DNS/conexão, TLS, HTTP, Content-Type e padrão de payload JSON.
- Adicionado `mrbe-healthcheck`: verifica framebuffer, entrada, rede, IP, rota, DNS, curl e launcher.
- Adicionado `mrbe-stability-test`: monitora launcher, memória disponível e RSS por padrão durante 5 minutos.
- Adicionada suíte local de API com servidor HTTP simulado (200, 503, JSON incorreto e Content-Type incorreto).
- `make check` passa a exigir os utilitários e o bundle CA na imagem.
- Nenhum endpoint comercial ou credencial de produção foi configurado.


## 0.6.0 — Home cinematográfica e assets visuais reais
- Redesenha a Home para a identidade MRBE aprovada: hero cinematográfico, relógio/Wi-Fi/perfil, recomendações em carrossel e linha de aplicativos.
- Adiciona 5 artes de recomendação, 9 tiles de aplicativos, 7 ícones e 7 rótulos de navegação com antialiasing.
- Instala os assets em `/usr/share/mrbe/ui/` no rootfs Buildroot.
- Reduz a barra lateral para 7 destinos: Home, Guia, Apps, Loja, Pesquisar, Biblioteca e Configurações.
- Atualiza a navegação por teclado/controle remoto e hit-testing do mouse para o novo layout.
- Otimiza o blit RGBA no framebuffer para desenhar arte de alta resolução sem depender de X11/Wayland.
- Smoke test passa a exigir `MRBE_HOME_ASSETS_READY`, além de boot, UI e evdev.
- Build ARM64 e boot QEMU 0.6.0 ainda dependem da recompilação no WSL do usuário.


## 0.5.1 — resolução 16:9, mouse e refinamento visual
- QEMU GUI solicita 1280x720 por EDID (xres/yres) e kernel mode setting (`video=Virtual-1`).
- Canvas 16:9 preserva proporção em framebuffers de fallback com letterboxing.
- Entrada evdev agrega teclado, mouse relativo, botão esquerdo/direito e roda.
- Cursor gráfico aparece ao mover o mouse; clique seleciona; botão direito volta; roda navega.
- Barra inferior encurtada para evitar corte horizontal.
- Testes de estado incluem hit-testing de mouse.
- Smoke test registra resolução solicitada.
- Testes QEMU reais continuam dependentes do WSL/Buildroot do usuário.

## 0.5.0 — Interface TV ampliada
- Home premium com navegação lateral.
- Apps, MRBE Store, Biblioteca, Detalhes, Filmes, Séries, Esportes, Infantil, Pesquisa e Configurações.
- Favoritos, recentes, filtros e restauração do foco ao voltar de Detalhes.
- Correção da navegação nos grids e dos filtros da tela Apps.
- Launcher automático no boot quando o framebuffer estiver disponível.
- Smoke test exige boot do userspace e primeira renderização da UI.
- QEMU GUI usa 4 GiB por padrão.

# Changelog

## 0.4.0 - 2026-11-06
- Adota o brasao MRBE com coroa, escudo, monitor/play e mascote Linux como identidade do sistema.
- Integra o asset RGBA transparente ao launcher framebuffer, sem biblioteca de imagem adicional.
- Atualiza paleta para carvão, verde-esmeralda, ouro e prata.
- Reaplica a identidade na Home, Apps, Configuracoes e painel de informacoes.
- Instala o brasao em `/usr/share/mrbe/brand/` no rootfs.
- Mantem Buildroot, kernel, QEMU e backend fbdev para preservar a base de boot previamente validada.
- Migração para LVGL fica como etapa seguinte, após validação visual e funcional da versão 0.4.0.

## 0.3.0 - 2026-11-05
- Nova interface home de TV estilo streaming, identidade MRBE original.
- Tela Apps, Configuracoes e atalhos H, A e S por evdev.
- Ilustracoes e cards procedurais em C, sem biblioteca grafica ou assets externos.
- Preview real das tres telas e testes automatizados do estado da UI.
- Rebuild inteligente do launcher Buildroot com hash de codigo-fonte; alvo `make ui-rebuild`.
- Script de compilacao corrige PATH importado do Windows e substitui uutils install localmente quando necessario.
- Apps, streaming e controles de energia sao demonstrativos.

# Changelog

## 0.2.0 — 2026-11-05
- Continua o DevKit 0.1.0 em um repositorio GitHub ready.
- Corrige configuracao Buildroot do kernel (defconfig arm64 + fragment absoluto).
- Adiciona boot marker `MRBE_BOOT_OK`, espera por framebuffer, imagens QEMU em snapshot.
- Adiciona smoke test de console serial com saida assim que detectar o boot, testes locais, gerador de previa do launcher.
- Documenta diferenca entre QEMU e a cadeia de boot do MXQ Pro; proibe flash prematuro.
- Documenta criterios verificaveis de uso em 1 GiB, escopo e limitacoes.

## 0.1.0 — DevKit original
- Bootstrap Buildroot, launcher simples framebuffer, console QEMU e guia iniciante.
