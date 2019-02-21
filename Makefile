OUT := debugcc

CC=aarch64-linux-gnu-gcc

CFLAGS := -O2 -Wall -g
LDFLAGS := 
prefix := /usr/local

SRCS := debugcc.c
OBJS := $(SRCS:.c=.o)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OUT) $(OBJS) *-debugcc
