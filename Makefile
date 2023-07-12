# FLAGS = -rdynamic -Wall -Wunused-function -Wextra -Werror
FLAGS = -rdynamic -Wall -Wunused-function -Wextra -Werror -finstrument-functions -finstrument-functions-exclude-file-list=src/profile.c,src/entrypoints/monitor.c

monitor: bin
	gcc -lncurses $(FLAGS) src/*.c src/entrypoints/monitor.c -o bin/monitor

test: bin
	gcc $(FLAGS) src/*.c src/entrypoints/test.c -o bin/test
	bin/test

nestest: bin
	gcc $(FLAGS) src/*.c src/entrypoints/nestest.c -o bin/nestest
	bin/nestest

test1000: bin
	gcc $(FLAGS) src/*.c src/entrypoints/test.c -o bin/test
	bin/test -n 1000

testerrors: bin
	gcc $(FLAGS) src/*.c src/entrypoints/test.c -o bin/test
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
