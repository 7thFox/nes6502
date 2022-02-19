monitor: bin
	gcc -lncurses -rdynamic -Wall -Wextra -Werror src/*.c src/entrypoints/monitor.c -o bin/monitor

test: bin
	gcc -rdynamic -Wall -Wunused-function -Wextra -Werror src/*.c src/entrypoints/test.c -o bin/test
	bin/test

test1000: bin
	gcc -rdynamic -Wall -Wunused-function -Wextra -Werror src/*.c src/entrypoints/test.c -o bin/test
	bin/test -n 1000

testerrors: bin
	gcc -rdynamic -Wall -Wunused-function -Wextra -Werror src/*.c src/entrypoints/test.c -o bin/test
	bin/test --errors-only

run: monitor
	bin/monitor

clean:
	rm -rf bin/

bin:
	mkdir bin

bin2rom: bin
	gcc src/entrypoints/bin2rom.c -o bin/bin2rom

ines2rom: bin
	gcc src/entrypoints/ines2rom.c -o bin/ines2rom
