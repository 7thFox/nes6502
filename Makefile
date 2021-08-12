clean:
	rm -rf bin/

bin:
	mkdir bin

build-monitor: bin
	gcc -lncurses src/*.c src/entrypoints/monitor.c -o bin/monitor

monitor: build-monitor
	bin/monitor