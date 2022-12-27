OUT := debugcc

CC=aarch64-linux-gnu-gcc

CFLAGS := -O2 -Wall -g
LDFLAGS := -static -static-libgcc
prefix := /usr/local

SRCS := debugcc.c \
	msm8936.c \
	msm8994.c \
	msm8996.c \
	qcs404.c \
	sc8280xp.c \
	sdm845.c \
	sm6115.c \
	sm6125.c \
	sm6350.c \
	sm8150.c \
	sm8250.c \
	sm8350.c \
	sm8450.c \
	sm8550.c
OBJS := $(SRCS:.c=.o)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	ln -f $(OUT) msm8936-debugcc
	ln -f $(OUT) msm8994-debugcc
	ln -f $(OUT) msm8996-debugcc
	ln -f $(OUT) qcs404-debugcc
	ln -f $(OUT) sc8280xp-debugcc
	ln -f $(OUT) sdm845-debugcc
	ln -f $(OUT) sm6115-debugcc
	ln -f $(OUT) sm6125-debugcc
	ln -f $(OUT) sm6350-debugcc
	ln -f $(OUT) sm8150-debugcc
	ln -f $(OUT) sm8250-debugcc
	ln -f $(OUT) sm8350-debugcc
	ln -f $(OUT) sm8450-debugcc
	ln -f $(OUT) sm8550-debugcc

$(OBJS): %.o: debugcc.h

clean:
	rm -f $(OUT) $(OBJS) *-debugcc
