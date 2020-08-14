SRCS=luci-metadata.c luci-process.c luci-element.c luci-utils.c
DEPS=luci.h
OBJS=$(SRCS:.c=.o)
TARGET=luci
LIBDIR=lib
PRODFLAGS=
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


debug:
		$(MAKE) PRODFLAGS=-DDEBUG=1  all


profile: PRODFLAGS+=-pg
profile: example

libluci.a: $(TARGET)
		ar rcs $(LIBDIR)/$@ $(OBJS)


libluci.so: $(TARGET)
		$(CC) -shared $(OBJS) -o $(LIBDIR)/$@

example: libluci.a
		$(CC) example.c -L$(LIBDIR) -l:libluci.a -o $(TARGET)

<<<<<<< HEAD
dynamicexample: libluci.so
		$(CC) example.c -L$(LIBDIR) -l:libluci.so -Wl,-rpath="$(LIBDIR)/" -o $(TARGET) # using -lluci is entirely possible but because both libs can exist we should differenciate
=======
example: libluci.so
		$(CC) example.c $(PRODFLAGS) -L$(LIBDIR) -lluci -Wl,-rpath="$(LIBDIR)/" -o $(TARGET)  
>>>>>>> fd557cfcfc27b716a49dbdb72fe2baa0bb2a617a


$(TARGET) : $(OBJS)
	mkdir -p $(LIBDIR)

clean:
	rm -rf $(OBJS) $(TARGET) example.o $(LIBDIR) a.out gmon.out
