# 08 — Orçamento e medição de memória

## Princípio

1 GiB é a memória total do guest. Não é um orçamento exclusivo do launcher.

## Metas iniciais (a validar)

- Sistema em repouso: alvo até 350 MiB.
- Sistema com launcher: alvo até 450 MiB.
- Player futuro: orçamento separado, inicialmente até 200 MiB.
- Manter reserva operacional para kernel, cache e picos.

Esses números são metas de engenharia, não medições da imagem atual.

## Medir

No guest:

```sh
cat /proc/meminfo
free
ps
```

Para observar processos:

```sh
ps -o pid,comm,rss,vsz
```

Se a versão BusyBox não oferecer essas opções, use `ps` e `/proc/<pid>/status`.

## Regras

- Não carregar navegador na inicialização.
- Não usar Electron.
- Não iniciar desktop completo.
- Evitar imagens grandes e múltiplas cópias de buffers.
- Liberar recursos ao sair de módulos.
- Avaliar zRAM somente depois de medir pressão de memória.
- Não usar swap em flash/eMMC como substituto de RAM sem avaliar desgaste e latência.

## Cuidado com resolução

Um framebuffer 1920x1080 em 32 bits ocupa cerca de 7,9 MiB por buffer. Dois buffers ocupam cerca de 15,8 MiB, sem contar outros buffers e estruturas. Em 1280x720, o custo é menor. Por isso, o primeiro teste gráfico é em resolução moderada.

O launcher desenha num back buffer em RAM do tamanho da área visível e copia o quadro pronto para `/dev/fb0`, para a tela nunca mostrar um quadro pela metade. Isso custa cerca de 3,5 MiB em 1280x720 e 7,9 MiB em 1920x1080. Se a alocação falhar, ele desenha direto no framebuffer, como antes.
