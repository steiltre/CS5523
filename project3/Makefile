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

all : knn

debug : knn_dbg

knn : knn.c
	$(CC) -o knn knn.c $(CCFLAGS)

knn_dbg : knn.c
	$(CC) -o knn knn.c $(DBGFLAGS)

clean:
	rm knn
