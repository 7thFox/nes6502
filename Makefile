clean:
	rm -rf bin/

bin:
	mkdir bin

monitor: bin
	gcc -lncurses src/*.c src/entrypoints/monitor.c -o bin/monitor

run-monitor: monitor
	bin/monitor

bin2rom: bin
	gcc src/entrypoints/bin2rom.c -o bin/bin2rom
