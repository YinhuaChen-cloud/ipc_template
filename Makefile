PHONY: run
run:
	gcc -g instrument.c -o instrument
	gcc -g fuzz.c -o fuzz
	./fuzz

PHONY: gdb
gdb:
	gcc -g instrument.c -o instrument
	gcc -g fuzz.c -o fuzz
	gdb ./fuzz

PHONY: clean
clean:
	-rm fuzz instrument

