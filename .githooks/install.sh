#!/bin/sh
# Ativação dos ganchos Git locais (.githooks)

set -e

HOOKS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$HOOKS_DIR/.." && pwd)"

chmod 0755 "$HOOKS_DIR/pre-commit"
chmod 0755 "$HOOKS_DIR/install.sh"

git -C "$REPO_ROOT" config core.hooksPath .githooks

printf "✓ Ganchos git ativados com sucesso em %s\n" "$REPO_ROOT"
