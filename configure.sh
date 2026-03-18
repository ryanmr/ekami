#!/usr/bin/env bash
# Reads .env.local (or .env.example as fallback) and generates main/config.h
# from main/config.h.in by substituting ${VAR} placeholders.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env.local"
TEMPLATE="$SCRIPT_DIR/main/config.h.in"
OUTPUT="$SCRIPT_DIR/main/config.h"

if [ ! -f "$ENV_FILE" ]; then
    echo "No .env.local found, falling back to .env.example"
    ENV_FILE="$SCRIPT_DIR/.env.example"
fi

if [ ! -f "$ENV_FILE" ]; then
    echo "Error: No .env.local or .env.example found" >&2
    exit 1
fi

if [ ! -f "$TEMPLATE" ]; then
    echo "Error: Template $TEMPLATE not found" >&2
    exit 1
fi

# Read env vars from file (skip blank lines and comments)
while IFS='=' read -r key value; do
    [[ -z "$key" || "$key" =~ ^# ]] && continue
    export "$key=$value"
done < "$ENV_FILE"

# Substitute ${VAR} patterns in the template
envsubst < "$TEMPLATE" > "$OUTPUT"

echo "Generated $OUTPUT from $ENV_FILE"
