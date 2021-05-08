CC=gcc
CFLAGS=-I. -I../../lib -D UNIX -D LINUX
LDFLAGS=

EXECUTABLE=demo_server

SOURCES= main.c \
shell.c \
../../lib/c_tty.c \
../../lib/loop_buff.c \
../../lib/c_cache.c \
../../lib/c_sh.c \
../../lib/c_sh_embed.c \
../../lib/c_sh_pars.c \
../../lib/nprintf.c

OBJECTS=$(notdir $(SOURCES:%.c=%.o))

all:  $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

%.o: ../../lib/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm $(EXECUTABLE) $(OBJECTS)


