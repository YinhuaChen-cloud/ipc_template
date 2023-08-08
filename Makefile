PHONY: run
run:
	gcc instrument.c -o instrument
	gcc fuzz.c -o fuzz
	./fuzz

PHONY: clean
clean:
	rm fuzz instrument

