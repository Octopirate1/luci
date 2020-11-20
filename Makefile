# CC=/usr/bin/afl-gcc
DEPS=luci.h
SRCDIR=src
OBJDIR=obj
BINDIR=bin
SRCS:=$(wildcard $(SRCDIR)/*.c)
INCS:=$(wildcard $(SRCDIR)/*.h)
OBJS=$(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET=$(BINDIR)/luci
LIBDIR=lib
PRODFLAGS=
LFLAGS=-Wall -I. -lm
CFLAGS=-DISPYTHON=0 -g -Wall -O0 -fPIC $(PRODFLAGS)


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

$(TARGET) : $(OBJS)
	mkdir -p $(BINDIR)
	mkdir -p $(LIBDIR)
#	$(CC) $(OBJS) $(LFLAGS) -o $@

$(OBJS) : $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

debug:
		$(MAKE) PRODFLAGS=-DDEBUG=1  all


profile: PRODFLAGS+=-pg
profile: example

afl: CC=/usr/bin/afl-gcc
afl: example

libluci.a: $(TARGET)
		ar rcs $(LIBDIR)/$@ $(OBJS)


libluci.so: $(TARGET)
		$(CC) -shared $(OBJS) -o $(LIBDIR)/$@

example: libluci.a
		$(CC) example.c -L$(LIBDIR) -l:libluci.a -o $(TARGET)

dynamicexample: libluci.so
		$(CC) example.c -L$(LIBDIR) -l:libluci.so -Wl,-rpath="$(LIBDIR)/" -o $(TARGET) # using -lluci is entirely possible but because both libs can exist we should differenciate

clean:
	rm -rf $(OBJDIR) $(BINDIR) example.o $(LIBDIR) a.out gmon.out
