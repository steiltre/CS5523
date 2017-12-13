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

all : knn regression nn_regression

debug : knn_dbg regression_dbg nn_regression_dbg

knn : knn.c
	$(CC) -o knn knn.c $(CCFLAGS)

knn_dbg : knn.c
	$(CC) -o knn knn.c $(DBGFLAGS)

regression : regression.c
	$(CC) -o regression regression.c $(CCFLAGS)

regression_dbg : regression.c
	$(CC) -o regression regression.c $(DBGFLAGS)

nn_regression : nn_regression.c
	$(CC) -o nn_regression nn_regression.c $(CCFLAGS)

nn_regression_dbg : nn_regression.c
	$(CC) -o nn_regression nn_regression.c $(DBGFLAGS)

clean:
	rm knn
	rm regression
	rm nn_regression
