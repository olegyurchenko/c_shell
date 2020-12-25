CC=gcc
CFLAGS=-I../../lib -D UNIX -D LINUX -D UNIX_CONSOLE
LDFLAGS=
OBJDIR=obj

SOURCES= main.c \
../../lib/c_cache.c \
../../lib/loop_buff.c \
../../lib/c_sh.c \
../../lib/c_tty.c \
../../lib/nprintf.c

#OBJECTS := $(patsubst ../../lib/%, ./%, $(SOURCES:.c=.o))
OBJECTS=$(notdir $(SOURCES:%.c=%.o))
#OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=tty

all:  $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

%.o: ../../lib/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm $(EXECUTABLE) $(OBJECTS)

test: $(EXECUTABLE)
	./$(EXECUTABLE)

