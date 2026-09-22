.POSIX:
.SILENT:
MAKEFLAGS += --no-print-directory -s

CC       ?= cc
STD      != $(CC) -std=c23 -E - < "/dev/null" > "/dev/null" 2>&1 && echo "-std=c23" || echo "-std=c2x"
CFLAGS   ?= -Wall -Wextra -Werror -pedantic -Wformat=2 -Wvla -Wshadow -Wconversion -Wsign-conversion -Wno-missing-field-initializers -Wno-cpp $(STD) -O2
LIBS     ?= -pthread

TARGETS   = recorder player
SRCS      = recorder.c player.c
HEADERS   = wav_oss.h

all: $(TARGETS)

recorder: recorder.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o recorder recorder.o $(LIBS)

player: player.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o player player.o

recorder.o: recorder.c $(HEADERS)
	$(CC) $(CFLAGS) -c recorder.c

player.o: player.c $(HEADERS)
	$(CC) $(CFLAGS) -c player.c

debug: CFLAGS = -Wall -Wextra -pedantic -Wformat=2 -Wvla -Wshadow -Wconversion -Wsign-conversion -Wno-missing-field-initializers -Wno-cpp $(STD) -g -O0 -fsanitize=address,undefined
debug: clean $(TARGETS)

check:
	$(CC) $(CFLAGS) -fsyntax-only $(SRCS)

clean:
	rm -f $(TARGETS) recorder.o player.o *.core

install: $(TARGETS)
	install -d /usr/local/bin
	install -m 0755 recorder /usr/local/bin/recorder
	install -m 0755 player /usr/local/bin/player
	if [ -f oss-audio.1 ]; then \
		install -d /usr/local/man/man1; \
		install -m 0644 oss-audio.1 /usr/local/man/man1/oss-audio.1; \
	fi

.PHONY: all debug check clean install
