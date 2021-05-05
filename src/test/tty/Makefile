CC=gcc
CFLAGS=-I../../lib -D UNIX -D LINUX -D UNIX_CONSOLE
LDFLAGS=

EXECUTABLE=tty

SOURCES= main.c \
../../lib/c_cache.c \
../../lib/loop_buff.c \
../../lib/c_sh.c \
../../lib/c_sh_embed.c \
../../lib/c_tty.c \
../../lib/nprintf.c

OBJECTS=$(notdir $(SOURCES:%.c=%.o))

all:  $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

%.o: ../../lib/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm $(EXECUTABLE) $(OBJECTS)

test: $(EXECUTABLE)
	./$(EXECUTABLE)

