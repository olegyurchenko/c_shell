CC=gcc
CFLAGS=-I../../lib -D UNIX_CONSOLE -D UNIX -D LINUX
LDFLAGS=

EXECUTABLE=shell

SOURCES= ../shell/main.c \
../shell/test_shell.c \
../../lib/loop_buff.c \
../../lib/c_tty.c \
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

%.o: ../shell/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm $(EXECUTABLE) $(OBJECTS)

test: $(EXECUTABLE)
	./$(EXECUTABLE) all_test.sh

