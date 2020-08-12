SRCS=example.c luci-metadata.c luci-process.c luci-element.c luci-utils.c
DEPS=luci.h
OBJS=$(SRCS:.c=.o)
TARGET=luci
PRODFLAGS=
CFLAGS=-DISPYTHON=0 -g -Wall -O0 $(PRODFLAGS)

ifeq ($(OS),Windows_NT) # Compiler-agnostic OS info
	PRODFLAGS += -D WIN32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
        	PRODFLAGS += -D LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
       		PRODFLAGS += -D OSX
	endif
endif

all: $(TARGET)

debug:
		$(MAKE) PRODFLAGS=-DDEBUG=1  all

profile:
		$(MAKE) PRODFLAGS=-pg all

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $^

clean:
	rm -rf $(OBJS) $(TARGET)
