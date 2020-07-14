
SRCS=luciexample.c luci.c
OBJS=$(SRCS:.c=.o)
TARGET=luci
DEFINES=-DNDEBUG=1
CFLAGS=-DISPYTHON=0 -g -Wall -DDEBUG=1 $(DEFINES)

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $^

clean:
	rm -rf $(OBJS) $(TARGET)
