CC=gcc
CFLAGS=-lreadline

build/printf_gen:
	mkdir -p build;
	$(CC) $(CFLAGS) src/*.c -o $@

.PHONY:install
install:build/printf_gen
	mkdir -p /usr/local/bin
	cp build/printf_gen /usr/local/bin/printf_gen

.PHONY:uninstall
uninstall:
	rm /usr/local/bin/printf_gen

.PHONY:clean
clean:
	rm -rf build
