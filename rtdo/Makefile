.POSIX:
.SILENT:
MAKEFLAGS += --no-print-directory -s

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Werror -pedantic -Wformat=2 -Wvla -Wshadow -Wconversion -Wsign-conversion -Wno-missing-field-initializers -Wno-cpp -std=c23 -O2
LIBS    ?= -lcrypt
TARGET   = rtdo
SRC      = rtdo.c
OBJ      = rtdo.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

rtdo.o: rtdo.c
	$(CC) $(CFLAGS) -c rtdo.c

debug: CFLAGS = -Wall -Wextra -pedantic -Wformat=2 -Wvla -Wshadow -Wconversion -Wsign-conversion -Wno-missing-field-initializers -Wno-cpp -std=c23 -g -O0 -fsanitize=address,undefined
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
	if [ -f rtdo.1 ]; then \
		install -d /usr/local/man/man1; \
		install -m 0644 rtdo.1 /usr/local/man/man1/rtdo.1; \
	fi

.PHONY: all debug check suid clean install
