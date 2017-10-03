#!/usr/bin/env python

import subprocess

#subprocess.call(["./dgt_compare", "~mnist_test.csv 28 28 1000"])

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_test.csv", "28", "28", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_test.csv", "28", "28", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_test.csv", "28", "28", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_5.csv", "5", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_5.csv", "5", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_5.csv", "5", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_5.csv", "5", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_5.csv", "5", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_5.csv", "5", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_10.csv", "10", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_10.csv", "10", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read()
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_10.csv", "10", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_10.csv", "10", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_10.csv", "10", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_10.csv", "10", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_20.csv", "20", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_20.csv", "20", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_20.csv", "20", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_20.csv", "20", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_20.csv", "20", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_20.csv", "20", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_40.csv", "40", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_40.csv", "40", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_U_40.csv", "40", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_40.csv", "40", "1", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_40.csv", "40", "1", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_US_40.csv", "40", "1", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_avg.csv", "7", "7", "10000", "1"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_avg.csv", "7", "7", "10000", "2"], stdout = subprocess.PIPE)
print proc.stdout.read(),
proc = subprocess.Popen(["./dgt_compare", "/home/trevor/mnist_avg.csv", "7", "7", "10000", "3"], stdout = subprocess.PIPE)
print proc.stdout.read()

