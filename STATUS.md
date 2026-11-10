# MRBE TV OS 0.7.0 — Estado da fase de estabilidade

## Identidade visual
A Home 0.6.0 aprovada permanece congelada, sem alteração de layout nesta fase.

## Implementado no código-fonte
- curl/libcurl com backend OpenSSL e bundle CA selecionados no Buildroot.
- `mrbe-healthcheck`, `mrbe-api-test` e `mrbe-stability-test` no rootfs overlay.
- Testes HTTP/HTTPS locais determinísticos, incluindo certificado confiável e não confiável.
- Monitoramento de MemAvailable, processo launcher e RSS por padrão durante 5 minutos.
- Smoke test exige marcador de presença do cliente API e bundle CA.

## Validação host
- `make api-test`: 13 testes aprovados.
- `make test`: 33 testes aprovados.
- Clang Static Analyzer: sem diagnósticos nos seis fontes C.
- AddressSanitizer/UndefinedBehaviorSanitizer: harness UI e 20 mil eventos aprovados.
- API pública: DNS não resolvido neste ambiente de revisão; teste externo pendente no QEMU do usuário.

## Pendente no ambiente do usuário
- Build ARM64 Buildroot 0.7.0 no WSL.
- `make check` e `make smoke` no WSL.
- Boot gráfico 0.7.0 no QEMU.
- `mrbe-healthcheck` e `mrbe-api-test` no guest com internet.
- Teste de estabilidade de 300 segundos no guest.

## Limites
- Nenhuma API comercial/streaming foi integrada.
- Endpoint padrão é JSONPlaceholder, apenas para testar transporte e payload JSON.
- Sem credenciais de produção.
- QEMU virt não é equivalente ao S905X3 físico.
