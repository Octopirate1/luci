
SRCS=luciexample.c luci.c luci_process_raw.c luci_element_array.c
DEPS=luci.h
OBJS=$(SRCS:.c=.o)
TARGET=luci
# DEFINES=-DNDEBUG=1 -DDEBUG=1
# DEFINES=
#PRODFLAGS=-DDEBUG=$(DEBUG)
PRODFLAGS=
CFLAGS=-DISPYTHON=0 -g -Wall -O0 $(PRODFLAGS)

all: $(TARGET)

debug:
		$(MAKE) PRODFLAGS=-DDEBUG=1 all

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $^

clean:
	rm -rf $(OBJS) $(TARGET)
