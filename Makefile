.POSIX:
.SILENT:

MAKEFLAGS += --no-print-directory -s

# ----------------------------------------------------------------
# Makefile: Core POSIX C99 & POSIX Implementations
# ----------------------------------------------------------------

SUBDIRS = rtdo rtgo

.PHONY: all help build debug check format clang-format prettier format-c format-docs format-check format-check-c format-check-docs clean install

all: help

### ================================
### HELP & DOCUMENTATION
### ================================
help:
	_e=$$'\e'; \
	cmd() { printf "    $${_e}[36mmake %-20s$${_e}[0m %s\n" "$$1" "$$2"; }; \
	sec() { printf "\n  $${_e}[1;33m%s$${_e}[0m\n" "$$1"; }; \
	sub() { printf "  $${_e}[1;34m  ── %s ──$${_e}[0m\n" "$$1"; }; \
	printf "\n  $${_e}[1;37mCore POSIX — Suíte de Utilitários em C99 / POSIX$${_e}[0m\n"; \
	printf "  ============================================================\n"; \
	sec "Compilação & Build:"; \
	cmd "build"          "Compila todos os utilitários (rtdo, rtgo)"; \
	cmd "debug"          "Compila em modo debug com símbolos (-g)"; \
	cmd "install"        "Instala os binários compilados no sistema"; \
	sec "Qualidade & Testes:"; \
	cmd "check"          "Executa testes unitários e valida formatação"; \
	cmd "format"         "Formata códigos C (clang-format) e docs (prettier)"; \
	cmd "clang-format"   "Formata arquivos C/Header com clang-format"; \
	cmd "prettier"       "Formata documentações e configs com Prettier"; \
	cmd "format-check"   "Verifica se a formatação está em conformidade"; \
	sec "Limpeza:"; \
	cmd "clean"          "Remove artefatos compilados em todos os subdiretórios"; \
	echo ""

build:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir all; \
	done

debug:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir debug; \
	done

check:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir check; \
	done
	$(MAKE) format-check

format: format-c format-docs

clang-format: format-c
prettier: format-docs

format-c:
	find . -type f \( -name "*.c" -o -name "*.h" \) -not -path "*/.*" -exec clang-format -i {} +

format-docs:
	if command -v prettier > "/dev/null" 2>&1; then \
		prettier --write "**/*.{md,json,jsonc,yml,yaml,html,css}" 2> "/dev/null" || true; \
	fi

format-check: format-check-c format-check-docs

format-check-c:
	find . -type f \( -name "*.c" -o -name "*.h" \) -not -path "*/.*" -exec clang-format --dry-run --Werror {} +

format-check-docs:
	if command -v prettier > "/dev/null" 2>&1; then \
		prettier --check "**/*.{md,json,jsonc,yml,yaml,html,css}"; \
	fi

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

install:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install; \
	done

.PHONY: all debug check format clang-format prettier format-c format-docs format-check format-check-c format-check-docs clean install
