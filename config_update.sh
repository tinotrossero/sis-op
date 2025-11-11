#!/bin/bash

# Verificar si se proporcionaron dos argumentos
if [ "$#" -ne 2 ]; then
  echo "Uso: $0 campo nuevo_valor"
  exit 1
fi

# Directorio actual
DIRECTORIO="."

# Campo y nuevo valor proporcionados como argumentos
CAMPO="$1"
NUEVO_VALOR="$2"

# Buscar y reemplazar en todos los archivos .config
find "$DIRECTORIO" -type f -name "*.config" -exec sed -i "s/^${CAMPO}[[:space:]]*=[[:space:]]*[^[:space:]]*/${CAMPO} = ${NUEVO_VALOR}/g" {} +

echo "Reemplazo completado."
