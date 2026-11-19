# Fase B — Boot: emulador x equipamento real

## QEMU nesta versao
Caminho real: `qemu-system-aarch64 -machine virt -kernel Image` -> Linux ->
rootfs ext4 virtio-blk -> BusyBox init -> `S05mrbe-launcher` -> interface framebuffer.
O launcher sobe antes da rede; `S99mrbe`, o ultimo script, so
imprime `MRBE_BOOT_OK`. O `/dev` vem do devtmpfs do kernel, sem mdev.
O QEMU entrega DTB generico da maquina virtual. **Nao carrega a BIOS do MXQ Pro**.

## Por que nao ha BIOS no TV Box
Em ARM SoCs, BootROM fisica -> boot stages do fabricante/BL2/SPL ->
U-Boot (ou bootloader proprio) -> DTB e kernel -> rootfs. BootROM e parte
do hardware e **nao e emulavel em `-M virt` sem modelo especifico**.

## Firmware UEFI em QEMU: marco posterior
Sera usado para avaliar a fase de firmware virtual, com `AAVMF`, particao
EFI e GRUB para ARM64. Nao confundir AAVMF com o bootloader do MXQ Pro.
O pipeline UEFI nao foi integrado nesta 0.2.0; nao existe `-bios` em `make gui`.

## Requisito de boot fisico
Para adicionar placa MXQ Pro: SoC identificado, RAM real, armazenamento,
DTB exato, firmware recuperavel e documentação de fonte de energia/serial.
O kernel/DTB da maquina `virt` **nao** sera gravado diretamente em eMMC/SD.

## Testes seguros progressivos
QEMU (sem hardware real) -> identifica placa -> backup comprovado ->
teste de boot **externo sem sobrescrever flash**, caso suportado ->
validacao de drivers -> homologacao. Se boot externo nao for suportado,
nao prossiga para flash.
