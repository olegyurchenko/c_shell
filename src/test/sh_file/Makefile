CC=gcc
CFLAGS=-I../../lib -D UNIX -D LINUX
LDFLAGS=

EXECUTABLE=sh_file

SOURCES= main.c \
../../lib/c_cache.c \
../../lib/c_sh.c \
../../lib/c_sh_embed.c \
../../lib/c_sh_pars.c \
../../lib/nprintf.c

OBJECTS=$(notdir $(SOURCES:%.c=%.o))

all:  test

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

%.o: ../../lib/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm $(EXECUTABLE) $(OBJECTS)

test: $(EXECUTABLE)
	./$(EXECUTABLE) all_test.sh

