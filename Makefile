CFLAGS=-O3 -std=c17 -std=gnu11 -Wall -Wextra -Wpedantic

.PHONY: all clean debug linux
all: main
main: $(wildcard *.c)	$(wildcard reference/*.c)#recognizes all C files in directory
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f main

debug: CFLAGS+=-g
debug: main

asan: CFLAGS+=-Werror -fsanitize=address
asan: main

ubsan: CFLAGS+=-Werror -fsanitize=undefined
ubsan: main

lsan: CFLAGS+=-Werror -fsanitize=leak
lsan: main

staticAnalysis: CFLAGS+=-Werror -fanalyzer
staticAnalysis: main
