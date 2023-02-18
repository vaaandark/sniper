cc=gcc
cflags=-Wall -Werror -Wshadow -O2
sniper: sniper.c
	$(cc) $(cflags) -o sniper sniper.c

sleep_and_print: sleep_and_print.c
	$(cc) $(cflags) -o sleep_and_print sleep_and_print.c

test: sniper sleep_and_print
	./sleep_and_print > /dev/null &
	./sniper `pgrep sleep_and_print`

clean:
	rm -f sniper sleep_and_print
