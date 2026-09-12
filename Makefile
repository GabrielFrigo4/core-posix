.POSIX:
.SILENT:

MAKEFLAGS += --no-print-directory -s

# ----------------------------------------------------------------
# Makefile: Core POSIX C99 & POSIX Implementations
# ----------------------------------------------------------------

SUBDIRS = rtdo rtgo

.PHONY: all help build debug check format format-c format-docs format-check format-check-c format-check-docs clean install

all: help

### ================================
### HELP & DOCUMENTATION
### ================================
help:
	cmd() { printf "    \033[36mmake %-20s\033[0m %s\n" "$$1" "$$2"; }; \
	sec() { printf "\n  \033[1;33m%s\033[0m\n" "$$1"; }; \
	sub() { printf "  \033[1;34m  ── %s ──\033[0m\n" "$$1"; }; \
	printf "\n  \033[1;37mCore POSIX — Suíte de Utilitários em C99 / POSIX\033[0m\n"; \
	printf "  ============================================================\n"; \
	sec "Compilação & Build:"; \
	cmd "build"          "Compila todos os utilitários (rtdo, rtgo)"; \
	cmd "debug"          "Compila em modo debug com símbolos (-g)"; \
	cmd "install"        "Instala os binários compilados no sistema"; \
	sec "Qualidade & Testes:"; \
	cmd "check"          "Executa testes unitários e valida formatação"; \
	cmd "format"         "Formata códigos C (clang-format) e docs (prettier)"; \
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

format-c:
	find . -type f \( -name "*.c" -o -name "*.h" \) -not -path "*/.*" -exec clang-format -i {} +

format-docs:
	if command -v prettier > "/dev/null" 2>&1; then \
		prettier --write "**/*.{md,yaml,yml,json}" 2> "/dev/null" || true; \
	fi

format-check: format-check-c format-check-docs

format-check-c:
	find . -type f \( -name "*.c" -o -name "*.h" \) -not -path "*/.*" -exec clang-format --dry-run --Werror {} +

format-check-docs:
	if command -v prettier > "/dev/null" 2>&1; then \
		prettier --check "**/*.{md,yaml,yml,json}"; \
	fi

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

install:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install; \
	done

.PHONY: all debug check format format-c format-docs format-check format-check-c format-check-docs clean install
