CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror
CPPFLAGS = -I../include
CORE = ../heapsort/heap.c ../heapsort/heapsort.c
HEADERS = ../include/heap.h ../include/heapsort.h

.PHONY: all test clean

all: bin/heapsort

bin:
	mkdir -p bin

bin/heapsort: main.c $(CORE) $(HEADERS) | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c $(CORE) -o $@

bin/teste_heap: tests/teste_heap.c $(CORE) $(HEADERS) | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/teste_heap.c $(CORE) -o $@

test: bin/heapsort bin/teste_heap
	./bin/teste_heap
	python3 -m unittest discover -s tests -p '*_test.py' -v

clean:
	rm -rf bin
