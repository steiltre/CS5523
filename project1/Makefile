CC = gcc
CCFLAGS = -march=native \
	-O3 \
	-lm \
	-Wall

debug: CCFLAGS += -g

all : fptminer
	$(CC) -o fptminer fptminer.c $(CCFLAGS)

clean:
	rm fptminer
