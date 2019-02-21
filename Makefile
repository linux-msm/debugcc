OUT := debugcc

CC=aarch64-linux-gnu-gcc

CFLAGS := -O2 -Wall -g
LDFLAGS := 
prefix := /usr/local

SRCS := debugcc.c qcs404.c
OBJS := $(SRCS:.c=.o)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	ln -f $(OUT) qcs404-debugcc

clean:
	rm -f $(OUT) $(OBJS) *-debugcc
