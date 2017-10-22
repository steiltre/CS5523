CC = gcc
CCFLAGS = -march=native \
	-lm \
	-Wall \
	-O2

DBGFLAGS = -march=native \
	-lm \
	-Wall \
	-g \
	-O0

all : fptminer

debug : fptminer_dbg

fptminer : fptminer.c
	$(CC) -o fptminer fptminer.c $(CCFLAGS)

fptminer_dbg : fptminer.c
	$(CC) -o fptminer fptminer.c $(DBGFLAGS)

clean:
	rm fptminer
