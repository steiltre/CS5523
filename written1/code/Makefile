CC = gcc
CCFLAGS = -lm -Wall -march=native

dgt_compare: CCFLAGS += -O2

debug: CCFLAGS += -g

dgt_compare: digit_utility.c digit.c digit_distance.c
	$(CC) *.c -o dgt_compare $(CCFLAGS)

debug: digit_utility.c digit.c digit_distance.c
	$(CC) *.c -o dgt_compare $(CCFLAGS)

clean:
	rm -f dgt_compare
