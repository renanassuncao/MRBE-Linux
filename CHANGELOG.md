# Changelog

## Nao lancado
- Launcher: retangulos recortados uma vez e preenchidos linha a linha, sem divisao por pixel em canais de 8 bits; redesenho completo cerca de 60x mais rapido no host (27,5 ms para 0,46 ms por quadro em 1280x720), com saida identica pixel a pixel.
- Launcher: desenho em back buffer e copia do quadro completo para `/dev/fb0`, sem quadros pela metade na tela.
- Launcher: espera bloqueante por teclas (sem acordar a cada 150 ms) e sem redesenho quando a tecla nao muda o estado.
- Corrige loop a 100% de CPU quando um teclado ou controle e desconectado.
- Testes de recorte do framebuffer, `mrbe_fb_present` e desconexao de entrada em `tests/test_fb.py`.
- Launcher poe o terminal virtual em modo grafico: o cursor do console nao pisca mais por cima da interface. A primeira tela fica marcada no `dmesg` (`mrbe-launcher: primeira tela`).

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
