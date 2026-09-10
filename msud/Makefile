.POSIX:
.SILENT:
MAKEFLAGS += --no-print-directory -s

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Werror -pedantic -std=c99 -O2
LIBS    ?= -lcrypt
TARGET   = msud
SRC      = msud.c
OBJ      = msud.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

msud.o: msud.c
	$(CC) $(CFLAGS) -c msud.c

debug: CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O0 -fsanitize=address,undefined
debug: clean $(TARGET)

check:
	$(CC) $(CFLAGS) -fsyntax-only $(SRC)

suid: $(TARGET)
	chown root $(TARGET)
	chmod 4755 $(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

install: $(TARGET)
	install -d /usr/local/bin
	install -m 4755 -o root $(TARGET) /usr/local/bin/$(TARGET)
	if [ -f msud.1 ]; then \
		install -d /usr/local/man/man1; \
		install -m 0644 msud.1 /usr/local/man/man1/msud.1; \
	fi

.PHONY: all debug check suid clean install
