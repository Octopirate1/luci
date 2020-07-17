
SRCS=luciexample.c luci.c luci_process_raw.c
OBJS=$(SRCS:.c=.o)
TARGET=luci
#DEFINES=-DNDEBUG=1
DEFINES=
CFLAGS=-DISPYTHON=0 -g -Wall -DDEBUG=1 $(DEFINES) -O0

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $^

clean:
	rm -rf $(OBJS) $(TARGET)
