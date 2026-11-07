# Changelog

## Nao lancado
- `make profile`: perfil de boot no QEMU com as initcalls mais lentas, o kernel ate o init, os maiores intervalos do console e a memoria com o launcher na tela.
- Launcher sobe em `S05mrbe-launcher`, antes da rede: a tela aparece em 1,8 s em vez de 4,5 s no QEMU, e sem cabo de rede nao espera mais o DHCP (eram 7,9 s).
- `/dev` vem do devtmpfs do kernel; sai o mdev, cujo coldplug gastava 2,2 s para carregar um modulo que nada usa. Sai o `crond`, que nao tinha crontab.
- `build.sh` reaplica o defconfig quando ele ou um fragment mudam e reinstala o rootfs; antes a mudanca nunca chegava a quem ja tinha compilado.
- Kernel com config sob medida para o QEMU virt (`board/qemu-aarch64/linux.config`), sem modulos: `Image` de 38,8 para 7,5 MB, kernel ate o init de 0,73 para 0,32 s, 71 MiB a mais de memoria livre, kernel compila em 55 s em vez de 372 s. Detalhes em `docs/22-KERNEL-SOB-MEDIDA.md`.
- `check-build.sh` falha se uma opcao do `linux.fragment` nao chegou a `.config` do kernel.

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
