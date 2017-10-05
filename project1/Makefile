CC = gcc
CCFLAGS = -march=native \
	-O0 \
	-lm \
	-Wall

debug: CCFLAGS += -g

debug : fptminer

all : fptminer

fptminer : fptminer.c
	$(CC) -o fptminer fptminer.c $(CCFLAGS)

clean:
	rm fptminer
