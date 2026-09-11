.POSIX:
.SILENT:
MAKEFLAGS += --no-print-directory -s

SUBDIRS = rtdo rtgo

all:
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
