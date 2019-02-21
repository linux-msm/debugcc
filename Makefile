OUT := debugcc

CC=aarch64-linux-gnu-gcc

CFLAGS := -O2 -Wall -g
LDFLAGS := 
prefix := /usr/local

SRCS := debugcc.c qcs404.c sdm845.c
OBJS := $(SRCS:.c=.o)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	ln -f $(OUT) qcs404-debugcc
	ln -f $(OUT) sdm845-debugcc

clean:
	rm -f $(OUT) $(OBJS) *-debugcc
