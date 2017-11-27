CC = gcc
CCFLAGS = -march=native \
	-lm \
	-Wall \
	-O2 \
	-fopenmp

DBGFLAGS = -march=native \
	-lm \
	-Wall \
	-g \
	-O0 \
	-fopenmp

all : kcluster

debug : kcluster_dbg

kcluster : kcluster.c
	$(CC) -o kcluster kcluster.c $(CCFLAGS)

kcluster_dbg : kcluster.c
	$(CC) -o kcluster kcluster.c $(DBGFLAGS)

preprocess :
	python preprocess.py

clean:
	rm kcluster
