#! /usr/bin/env python

import subprocess

trainfnames = ["./data/rep1/mnist_train.csv", "./data/rep2/mnist_train.csv", "./data/rep3/mnist_train.csv"]
validfnames = ["./data/rep1/mnist_validation.csv", "./data/rep2/mnist_validation.csv", "./data/rep3/mnist_validation.csv"]
testfnames = ["./data/rep1/mnist_test.csv", "./data/rep2/mnist_test.csv", "./data/rep3/mnist_test.csv"]

outfname = "output.txt"
wgtfname = "weights.csv"

knn_acc = []
reg_acc = []
knn_val = []
reg_val = []
knn_cls = []
reg_cls = []

nn_acc = []
nn_val = []
nn_cls = []

for i in range(0,len(trainfnames)):
    proc = subprocess.Popen(["./knn", trainfnames[i], validfnames[i], testfnames[i], outfname], stdout=subprocess.PIPE)
    output = proc.stdout.readlines()

    acc = output[0].split(":")[1].strip()
    val = output[1].split(":")[1].strip()
    cls = output[2].split(":")[1].strip()

    knn_acc.append(acc)
    knn_val.append(val)
    knn_cls.append(cls)

for i in range(0,len(trainfnames)):
    proc = subprocess.Popen(["./regression", trainfnames[i], validfnames[i], testfnames[i], outfname, wgtfname], stdout=subprocess.PIPE)
    output = proc.stdout.readlines()

    acc = output[0].split(":")[1].strip()
    val = output[1].split(":")[1].strip()
    cls = output[2].split(":")[1].strip()

    reg_acc.append(acc)
    reg_val.append(val)
    reg_cls.append(cls)

proc = subprocess.Popen(["./nn_regression", trainfnames[i], validfnames[i], testfnames[i], outfname, wgtfname], stdout=subprocess.PIPE)
output = proc.stdout.readlines()

acc = output[0].split(":")[1].strip()
val = output[1].split(":")[1].strip()
cls = output[2].split(":")[1].strip()

nn_acc.append(acc)
nn_val.append(val)
nn_cls.append(cls)

print '\\begin{figure}[h]'
print '\t\\begin{tabular}{| r | r r r | r r r |}'
print '\t\t\\hline'
print '\t\t\\multicolumn{7}{| c | }{Classification Results} \\\\'
print '\t\t\\& multicolumn{3}{| c |}{KNN} & \\multicolumn{3}{| c |}{Regression} \\\\'
print '\t\tData Representation & Accuracy & Validation Time (sec) & Classification Time (sec) & Accuracy & Validation Time (sec) & Classification Time (sec) \\\\'
print '\t\tRep1 & {} & {} & {} & {} & {} & {} \\\\'.format(knn_acc[0], knn_val[0], knn_cls[0], reg_acc[0], reg_val[0], reg_cls[0])
print '\t\t\\hline'
print '\t\tRep2 & {} & {} & {} & {} & {} & {} \\\\'.format(knn_acc[1], knn_val[1], knn_cls[1], reg_acc[1], reg_val[1], reg_cls[1])
print '\t\t\\hline'
print '\t\tRep3 & {} & {} & {} & {} & {} & {} \\\\'.format(knn_acc[2], knn_val[2], knn_cls[2], reg_acc[2], reg_val[2], reg_cls[2])
print '\t\t\\hline'
print '\t\t\\end{tabular}'
print '\t\\caption{Classification results using K-Nearest Neighbors and Ridge Regression'
print '\t\\label{fig:res}'
print '\t\\end{figure}'

print '\\begin{figure}[h]'
print '\t\\begin{tabular}{| r | r r r|}'
print '\t\t\\hline'
print '\t\t\\multicolumn{4}{| c |}{Non-negative Ridge Regression Results} \\\\'
print '\t\t\\Data Representation & Accuracy & Validation Time (sec) & Classification Time (sec) \\\\'
print '\t\tRep1 & {} & {} & {} \\\\'.format(nn_acc[0], nn_val[0], nn_cls[0])
print '\t\t\\hline'
print '\t\t\\end{tabular}'
print '\t\\caption{Classification results using Non-Negative Ridge Regression'
print '\t\\label{fig:res2}'
print '\t\\end{figure}'
