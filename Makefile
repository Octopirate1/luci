# CC=/usr/bin/afl-gcc # for fuzzing with "afl-fuzz -i test/ -o results/ ./luci @@"
SRCS=example.c luci-metadata.c luci-process.c luci-element.c luci-utils.c
DEPS=luci.h
OBJS=$(SRCS:.c=.o)
TARGET=luci
PRODFLAGS=
CFLAGS=-DISPYTHON=0 -g -Wall -O0 $(PRODFLAGS)

all: $(TARGET)

debug:
		$(MAKE) PRODFLAGS=-DDEBUG=1  all

profile:
		$(MAKE) PRODFLAGS=-pg all

fuzz: CC=/usr/local/bin/afl-clang
fuzz: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $^

clean:
	rm -rf $(OBJS) $(TARGET)
