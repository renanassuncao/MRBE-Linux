# MRBE TV OS 0.7.0 — estabilidade e comunicação de APIs

## Objetivo

Manter a identidade visual 0.6.0 congelada e acrescentar verificações repetíveis de sistema, rede, HTTP/HTTPS, TLS, resposta JSON e estabilidade do launcher.

## O que esta versão realmente testa

- `mrbe-healthcheck`: framebuffer, dispositivos evdev, interface eth0, IPv4, rota padrão, DNS, curl e processo do launcher.
- `mrbe-api-test`: resolução/conexão via curl, validação de certificado TLS (sem `-k`), status HTTP esperado, Content-Type JSON, corpo não vazio e padrão configurável no payload.
- `mrbe-stability-test`: por padrão, 5 minutos de monitoramento do processo do launcher, MemAvailable e crescimento de RSS.
- Testes host usam um servidor HTTP local controlado para validar HTTP 200, HTTP 503, payload inesperado e Content-Type incorreto.

## Dependências adicionadas ao Buildroot

- curl/libcurl;
- backend TLS OpenSSL;
- bundle de certificados CA.

O build verifica que os binários e `/etc/ssl/certs/ca-certificates.crt` foram incluídos no rootfs.

## Testes no host (WSL ou Linux)

```bash
make api-test
make test
```

`make api-test` não depende da internet: usa um servidor mock local. `make test` executa a suíte completa, compilação C nativa e geração de prévias.

## Testes dentro do QEMU

Após atualizar os fontes, o script de build mescla as opções de curl/TLS/CA no `.config` existente da versão 0.6.x antes de executar `olddefconfig`; assim, preservamos a pasta `work/` sem deixar as novas dependências de fora.

Reconstrua e inicie a imagem:

```bash
make ui-rebuild
make check
make smoke
make gui
```

No console serial do guest, faça login como `root` e execute:

```sh
mrbe-healthcheck
mrbe-api-test
MRBE_STABILITY_DURATION=300 mrbe-stability-test
```

O endpoint padrão de diagnóstico é `https://jsonplaceholder.typicode.com/todos/1`, um endpoint público de demonstração, não uma API de streaming. Para testar outro endpoint GET:

```sh
MRBE_API_URL="https://seu-servidor/health" \
MRBE_API_EXPECT_PATTERN='"status"[[:space:]]*:[[:space:]]*"ok"' \
mrbe-api-test
```

Para uma API com CA privada, informe o certificado de CA sem desativar a verificação TLS:

```sh
MRBE_API_URL="https://api.interna/health" \
MRBE_API_CAINFO="/etc/ssl/certs/minha-ca.pem" \
mrbe-api-test
```

Para executar uma verificação de API a cada 60 segundos durante o teste de estabilidade:

```sh
MRBE_STABILITY_DURATION=300 \
MRBE_STABILITY_API_INTERVAL=60 \
mrbe-stability-test
```

Parâmetros de estabilidade:

- `MRBE_STABILITY_DURATION`: duração em segundos (padrão 300).
- `MRBE_STABILITY_INTERVAL`: intervalo entre amostras (padrão 10).
- `MRBE_STABILITY_MIN_MEM_KB`: MemAvailable mínimo (padrão 65536 KB).
- `MRBE_STABILITY_MAX_RSS_GROWTH_KB`: crescimento máximo do RSS do launcher (padrão 65536 KB).
- `MRBE_STABILITY_API_INTERVAL`: intervalo de API; 0 desabilita (padrão 0).

## Limites e interpretação

- O teste padrão comprova comunicação HTTP/HTTPS com uma API pública de demonstração. Não comprova autenticação ou integração com Netflix, Disney+, Prime Video, Max, nem serviços comerciais.
- Nenhuma chave, token ou URL privada foi fornecida; credenciais não são gravadas nos scripts nem nos logs.
- Testes host mock não substituem a execução real dentro do QEMU.
- O monitor de RSS é uma heurística de estabilidade, não uma prova matemática de ausência de vazamento de memória.
- O QEMU `virt` AArch64 não comprova compatibilidade com a placa física S905X3.

## Critérios de aceite da fase

1. `make test` aprovado no host.
2. Buildroot conclui e `make check` encontra curl, certificados CA e os três utilitários.
3. `make smoke` confirma boot, assets, UI, entrada e presença do cliente API + bundle CA (marcador `MRBE_API_TOOLS_READY`); ele não executa uma chamada externa de API.
4. `mrbe-healthcheck` sem falha crítica no guest.
5. `mrbe-api-test` retorna `[PASS]` no guest com rede externa ativa.
6. `mrbe-stability-test` conclui 300 segundos sem queda do launcher nem violação dos limites de memória.
7. Relatórios e logs são anexados ao registro da versão.
