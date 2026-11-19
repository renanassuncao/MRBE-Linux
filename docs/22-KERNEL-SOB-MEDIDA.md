# 22 — Kernel sob medida para o QEMU virt

O kernel 6.12.27 do QEMU usa `board/qemu-aarch64/linux.config`, uma config feita
para esta máquina: QEMU `virt`, Cortex-A53, virtio e o launcher em framebuffer.
Antes era o defconfig arm64, que liga drivers de todos os SoCs ARM64 que o Linux
suporta: 3.178 opções embutidas e 1.159 módulos, quase nada usado aqui.

## Resultado

Mesmo rootfs e mesmo launcher, QEMU TCG com 1 GiB, mediana de 3 boots:

| | defconfig arm64 | sob medida |
|---|---|---|
| `Image` | 38,8 MB | 7,5 MB |
| Módulos em `/lib/modules` | 67 MiB | nenhum |
| `output/target` inteiro | 70 MiB | 3,2 MiB |
| Kernel até `/sbin/init` | 0,73 s | 0,32 s |
| Initcalls | 1.803 | 233 |
| `MRBE_BOOT_OK` | 1,58 s | 0,96 s |
| Memória disponível com o launcher na tela | 896 MiB | 967 MiB |
| Código do kernel residente | 17,3 MiB | 4,7 MiB |
| Reserva CMA | 32 MiB | nenhuma |
| Compilação do kernel (`make linux-dirclean && make linux`, 22 threads) | 372 s, 7.221 s de CPU | 55 s, 717 s de CPU |

O tempo de CPU é o que conta no WSL com `JOBS=2`. Sob TCG os tempos servem para
comparar versões, não para prever o hardware real.

## Como foi feita

1. `make allnoconfig` com `KCONFIG_ALLCONFIG` apontando para a lista do que o
   sistema usa: tudo desligado, menos essa lista e o que ela exige.
2. Boot no QEMU e conferência de launcher, teclado, rede, RTC e desligamento.
   Opções que o kernel liga por padrão e o `allnoconfig` desliga entraram
   explicitamente. Por exemplo, `FB_DEVICE` (sem ela não existe `/dev/fb0`) e
   `DRM_VIRTIO_GPU_KMS` (sem ela o virtio-gpu não registra o framebuffer).
3. `make savedefconfig` gera `linux.config`, só com o que difere dos padrões.
   Esse arquivo mais `linux.fragment`, pelo caminho do Buildroot (cópia, fragment
   e `olddefconfig`), reproduz a mesma `.config`.

`linux.fragment` continua sendo a lista do que o boot exige. Ele também fixa os
padrões do kernel de que o sistema depende, para que uma config nova não os perca.

## O que está embutido

| Grupo | Opções | Por quê |
|---|---|---|
| CPU | `SMP`, `NR_CPUS=4`, errata 826319, 827319, 824069, 819472 e 843419 do Cortex-A53 | `-smp 2` no QEMU e o A53 do MXQ Pro |
| Agendador | `PREEMPT_VOLUNTARY`, `NO_HZ_IDLE`, `HIGH_RES_TIMERS` | Resposta ao teclado sem o custo da preempção total; sem tick em repouso |
| Endurecimento | `RANDOMIZE_BASE` (KASLR), `STACKPROTECTOR_STRONG`, `VMAP_STACK` | Custo desprezível; o QEMU entrega a semente do KASLR no DTB |
| Diagnóstico | `PRINTK_TIME`, `KALLSYMS` | Tempos e nomes das initcalls em `make profile` e nos oops |
| Userspace | `BINFMT_SCRIPT`, `UNIX98_PTYS`, `DEVTMPFS_MOUNT`, `TMPFS`, `PROC_FS`, `SYSFS` | Scripts de init, login serial, `/dev` sem mdev, `/tmp` e `/run` |
| Barramentos | `PCI_HOST_GENERIC`, `VIRTIO_PCI`, `VIRTIO_MMIO` | GPU, teclado e mouse são virtio-pci; disco e rede são virtio-mmio |
| Disco | `VIRTIO_BLK`, `EXT4_FS` | rootfs ext4 em `/dev/vda` |
| Vídeo | `DRM_VIRTIO_GPU(_KMS)`, `DRM_FBDEV_EMULATION`, `FB_DEVICE`, `FRAMEBUFFER_CONSOLE` | `/dev/fb0` do launcher e console de texto na tela |
| Entrada | `INPUT_EVDEV`, `VIRTIO_INPUT` | `/dev/input/event*` |
| Rede | `INET`, `PACKET`, `UNIX`, `VIRTIO_NET` | IPv4, DHCP do udhcpc (AF_PACKET), `/dev/log` do syslogd; base para o HTTPS da 0.7.0 |
| Relógio | `RTC_DRV_PL031`, `RTC_HCTOSYS` | Hora certa desde o boot: certificados TLS têm validade |

## O que ficou de fora e quando religar

| Fora | Efeito | Religar quando |
|---|---|---|
| Módulos (`MODULES`) | Tudo embutido; nada para carregar no boot | Um driver precisar ser carregado depois do boot |
| USB e HID | Teclado USB não funciona; o QEMU usa teclado virtio | Teclado ou controle USB |
| IPv6 | Só IPv4 | Rede só IPv6 |
| `IP_MULTICAST` | Sem mDNS, SSDP ou DLNA | Descoberta de dispositivos na rede local |
| EFI e ACPI | Boot só com `-kernel` e DTB | Marco 0.5.0 (UEFI com AAVMF e GRUB): `EFI`, `EFI_STUB` |
| `SECCOMP`, cgroups, namespaces | Navegadores e contêineres não rodam | Chromium, Flatpak, systemd |
| `SWAP` | Sem swap e sem zram | Depois de medir pressão de memória (doc 08) |
| `PM`, `SUSPEND`, `CPU_IDLE`, `CPU_FREQ`, `THERMAL` | Sem suspensão nem controle de frequência; o QEMU não tem | No hardware real (standby da TV) |
| `PERF_EVENTS` | Sem `perf` | Sessões de profiling |
| `RODATA_FULL_DEFAULT_ENABLED` | Mapeamento linear em blocos grandes, menos falhas de TLB | Endurecimento acima do desempenho |
| CMA | 32 MiB que voltam para o sistema; o virtio-gpu não precisa | Controlador de vídeo real que exija memória contígua |

## Decisões medidas

- `-O2` (`CC_OPTIMIZE_FOR_PERFORMANCE`) ficou. Com `-Os` o `Image` cai de 7,5
  para 6,5 MB, mas o boot fica 7% mais lento (initcalls de 188 para 205 ms).
- Com o kernel enxuto, sobram as initcalls de dispositivos: `virtio_gpu` (41 ms),
  a criação dos dispositivos do DTB (20 ms) e `chr_dev_init` (14 ms). Depois do
  `/sbin/init`, o tempo é do userspace, e o maior item é o DHCP.

## Não é a config do MXQ Pro

A placa real precisa de outra config: drivers do SoC Amlogic, MMC, USB,
Ethernet ou Wi-Fi, HDMI, CMA, controle de frequência e temperatura. O método vale
para ela: lista do que o sistema usa, `allnoconfig`, boot, medição com
`make profile` e `savedefconfig`.

## Mudar a config

```bash
make -C work/buildroot-2025.02.19 O="$PWD/work/output" linux-menuconfig
make -C work/buildroot-2025.02.19 O="$PWD/work/output" linux-update-defconfig
```

O segundo comando regrava `linux.config` em formato savedefconfig, sem o
cabeçalho. Se a opção for algo de que o sistema depende, coloque-a também em
`linux.fragment`. Depois, `make build`, `make smoke` e `make profile`.
