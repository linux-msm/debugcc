OUT := debugcc

CC=aarch64-linux-gnu-gcc

CFLAGS := -O2 -Wall -g
LDFLAGS := -static -static-libgcc
prefix := /usr/local

SRCS := debugcc.c qcs404.c sdm845.c sm8350.c sm8450.c
OBJS := $(SRCS:.c=.o)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	ln -f $(OUT) qcs404-debugcc
	ln -f $(OUT) sdm845-debugcc
	ln -f $(OUT) sm8350-debugcc
	ln -f $(OUT) sm8450-debugcc

$(OBJS): %.o: debugcc.h

clean:
	rm -f $(OUT) $(OBJS) *-debugcc
