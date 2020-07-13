
SRCS=luciexample.c
OBJS=$(SRCS:.c=.o)
TARGET=luci
CFLAGS=-DISPYTHON=0 -g

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $^


