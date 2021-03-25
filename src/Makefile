CC := gcc
SRCS := $(wildcard *.c)
OBJS := $(SRCS:c=o)
INCLUDES = -I./
.PHONY: all clean
all: app 
UNAME_S := $(shell uname -s)
CFLAGS+= -g
ifneq ($(UNAME_S),Darwin)
	LDFLAGS+= -lasan
	CFLAGS+= -fsanitize=address -fno-common
endif

app: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): %.o: %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES)

clean:
	@rm -rf app $(OBJS)
