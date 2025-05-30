# FLAGS = -rdynamic -Wall -Wunused-function -Wextra -Werror -Wno-unused-parameter
FLAGS = -rdynamic \
	-Wall -Wextra -Werror \
	-Wno-comment \
	-Wunused-function \
	-Wno-unused-parameter \
	-Wno-unused-variable \
	-finstrument-functions -finstrument-functions-exclude-file-list=src/profile.c,src/entrypoints/monitor.c

monitor-ncurses: bin
	gcc -lncurses $(FLAGS) src/*.c src/entrypoints/monitor.c -o bin/monitor-ncurses

monitor-sdl: bin
	gcc -lSDL2 -lSDL2_ttf -lSDL2_image $(FLAGS) src/*.c src/entrypoints/sdl_monitor.c -o bin/monitor

test: bin
	gcc $(FLAGS) src/*.c src/entrypoints/test.c -o bin/test
	bin/test

nestest: bin
	gcc $(FLAGS) src/*.c src/entrypoints/nestest.c -o bin/nestest
	bin/nestest

dis: bin
	gcc $(FLAGS) src/*.c src/entrypoints/disassembler.c -o bin/dis
	bin/dis

test1000: bin
	gcc $(FLAGS) src/*.c src/entrypoints/test.c -o bin/test
	bin/test -n 1000

testerrors: bin
	gcc $(FLAGS) src/*.c src/entrypoints/test.c -o bin/test
	bin/test --errors-only

run: monitor-sdl
	bin/monitor

run-ncurses: monitor-ncurses
	bin/monitor-ncurses

clean:
	rm -rf bin/

bin:
	mkdir bin

bin2rom: bin
	gcc src/entrypoints/bin2rom.c -o bin/bin2rom

ines2rom: bin
	gcc src/entrypoints/ines2rom.c -o bin/ines2rom
