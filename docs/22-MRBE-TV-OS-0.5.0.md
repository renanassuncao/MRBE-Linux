# MRBE TV OS 0.5.0 — Implementação visual para QEMU

## Objetivo
Evoluir o launcher framebuffer 0.4.0 para uma interface de Smart TV com identidade MRBE, mantendo o Buildroot ARM64 e sem dependências gráficas externas.

## Telas implementadas no código
- Home: sidebar, status, hero original, streaming shortcuts, conteúdo ilustrativo.
- Apps: grade de aplicativos e detalhes.
- MRBE Store: catálogo visual e filtros por categoria.
- Biblioteca: favoritos e aplicativos visualizados durante a sessão.
- Detalhes: informações e estado de compatibilidade.
- Filmes, Séries, Esportes e Infantil: grades demonstrativas.
- Busca: tela e categorias; entrada de texto ainda não implementada.
- Configurações: rede, vídeo, áudio, controle, apps, armazenamento, atualização e sobre.
- Painel rápido: atalhos informativos; controles de hardware ainda não conectados.

## Navegação
- Setas: mover foco.
- Enter: selecionar.
- Esc/Backspace: voltar.
- H: Home.
- A: Apps.
- S: Configurações.
- Q: Busca.
- M: Painel rápido.
- F: adicionar/remover favorito.

## Limitações funcionais
- Netflix, Disney+, Prime Video e Max são cartões/catálogo demonstrativos. Não há APKs, DRM, certificação, autenticação ou reprodução.
- A MRBE Store ainda não baixa nem instala pacotes.
- Busca por texto ainda não está implementada.
- Configurações mostram estado/informações; não alteram efetivamente rede, áudio ou resolução.
- Favoritos e itens visualizados ficam apenas na memória da sessão.
- Conteúdo de Filmes/Séries/Esportes/Infantil é ilustrativo.
- O backend gráfico segue framebuffer direto. Esta versão ainda não migrou para LVGL.

## Testes
Executar no Ubuntu/WSL:
```bash
make test
JOBS=2 make ui-rebuild
make check
make smoke
make gui
```

QEMU GUI e smoke usam 4096 MiB e 4 CPUs por padrão. Para host com menos memória:
```bash
RAM_MB=1024 SMP=2 make gui
RAM_MB=1024 SMP=2 make smoke
```

O modo snapshot continua preservando a imagem base entre sessões.

## Diagnóstico se a janela abrir sem interface
Dentro do guest, no console:
```sh
cat /var/log/mrbe-launcher.log
ls -l /dev/fb0
mrbe-diagnostico
```

Se `/dev/fb0` não existir, investigar o driver VirtIO GPU e a configuração do kernel. `MRBE_BOOT_OK` confirma init, não confirma o launcher gráfico.

## Estado
Implementação de fonte 0.5.0 concluída para a trilha framebuffer/QEMU. Build ARM64 e execução visual QEMU precisam ser repetidos no WSL do usuário; não são declarados aprovados por este pacote.
