# Manifesto — MRBE TV OS 0.7.0

## Incluído
- Identidade visual 0.6.0 congelada.
- curl/libcurl com OpenSSL e certificados CA selecionados no Buildroot.
- Diagnóstico de sistema e rede (`mrbe-healthcheck`).
- Teste de API HTTP/HTTPS (`mrbe-api-test`).
- Monitor de estabilidade (`mrbe-stability-test`).
- Smoke marker para presença das ferramentas API/CA.
- Mesclagem automática das novas opções curl/TLS/CA no `.config` 0.6.x existente, preservando demais configurações.
- 13 testes específicos de API/estabilidade; 33 testes totais aprovados no host.
- Testes TLS com CA confiável e rejeição de certificado não confiável.
- Análise estática C, ASan/UBSan e teste de navegação com 20 mil eventos.
- Documentação e evidências de execução.

## Ainda não comprovado
- Build ARM64 no WSL do usuário.
- Boot QEMU 0.7.0.
- Comunicação externa do guest (DNS não disponível no ambiente host desta revisão).
- Teste de estabilidade de 5 minutos no QEMU.

## Fora do escopo desta versão
- APIs privadas de streaming, autenticação de serviços comerciais, DRM/Widevine, instalação de APKs e suporte S905X3 físico.
