.POSIX:
.SILENT:
MAKEFLAGS += --no-print-directory -s

CC      ?= cc
STD     ?= $(shell $(CC) -std=c23 -E - < /dev/null >/dev/null 2>&1 && echo "-std=c23" || echo "-std=c2x")
CFLAGS  ?= -Wall -Wextra -Werror -pedantic -Wformat=2 -Wvla -Wshadow -Wconversion -Wsign-conversion -Wno-missing-field-initializers -Wno-cpp $(STD) -O2
LIBS    ?=
TARGET   = rtgo
SRC      = rtgo.c
OBJ      = rtgo.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

rtgo.o: rtgo.c
	$(CC) $(CFLAGS) -c rtgo.c

debug: CFLAGS = -Wall -Wextra -pedantic -Wformat=2 -Wvla -Wshadow -Wconversion -Wsign-conversion -Wno-missing-field-initializers -Wno-cpp $(STD) -g -O0 -fsanitize=address,undefined
debug: clean $(TARGET)

check:
	$(CC) $(CFLAGS) -fsyntax-only $(SRC)

suid: $(TARGET)
	chown root:wheel $(TARGET) 2>/dev/null || chown root $(TARGET)
	chmod 4750 $(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

install: $(TARGET)
	install -d /usr/local/bin
	install -m 4750 -o root $(TARGET) /usr/local/bin/$(TARGET)
	chgrp wheel /usr/local/bin/$(TARGET) 2>/dev/null || true
	if [ -f rtgo.1 ]; then \
		install -d /usr/local/man/man1; \
		install -m 0644 rtgo.1 /usr/local/man/man1/rtgo.1; \
	fi

.PHONY: all debug check suid clean install
