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

format:
	find . -type f \( -name "*.c" -o -name "*.h" \) -not -path "*/.*" -exec clang-format -i {} +

format-check:
	find . -type f \( -name "*.c" -o -name "*.h" \) -not -path "*/.*" -exec clang-format --dry-run --Werror {} +

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

install:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install; \
	done

.PHONY: all debug check format format-check clean install
