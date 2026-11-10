#!/usr/bin/env bash
# Merge MRBE 0.7.0 API/TLS selections into an existing Buildroot .config.
# This preserves unrelated user settings in work/output/.config.
set -Eeuo pipefail
CONFIG_FILE="${1:?Usage: ensure-api-config.sh path/to/.config}"
[ -f "$CONFIG_FILE" ] || { echo "[ERRO] Buildroot config not found: $CONFIG_FILE" >&2; exit 2; }

# Remover simbolos antigos que ativam BR2_LEGACY no Buildroot 2025.02.x.
sed -i \
  -e '/^BR2_PACKAGE_CURL=/d' \
  -e '/^# BR2_PACKAGE_CURL is not set$/d' \
  -e '/^BR2_PACKAGE_LIBCURL_BEARSSL=/d' \
  -e '/^# BR2_PACKAGE_LIBCURL_BEARSSL is not set$/d' \
  "$CONFIG_FILE"

ensure_config_option() {
    local option="$1"
    local symbol="${option%%=*}"
    if grep -q "^${symbol}=" "$CONFIG_FILE"; then
        sed -i "s|^${symbol}=.*|${option}|" "$CONFIG_FILE"
    else
        sed -i "/^# ${symbol} is not set$/d" "$CONFIG_FILE"
        printf '%s\n' "$option" >> "$CONFIG_FILE"
    fi
}

for option in \
    'BR2_PACKAGE_OPENSSL=y' \
    'BR2_PACKAGE_LIBCURL=y' \
    'BR2_PACKAGE_LIBCURL_OPENSSL=y' \
    'BR2_PACKAGE_LIBCURL_CURL=y' \
    'BR2_PACKAGE_CA_CERTIFICATES=y'; do
    ensure_config_option "$option"
done

echo '[MRBE] Opcoes curl/OpenSSL/CA mescladas no Buildroot .config.'
