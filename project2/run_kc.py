#!/usr/bin/env python

import subprocess
import os

print os.getcwd()

ifnames = ["./freq.csv", "./sqrtfreq.csv", "./log2freq.csv"]
#ifnames = ["./2d_test.csv"]
classname = "./reuters21578.class"
#classname = "./2d_test.class"
crit_funcs = ["SSE", "I2", "E1"]
num_clusters = [20, 40, 60]
#num_clusters = [2, 3]
num_trials = 20
outfile = "./test.csv"

output_crit_funcs = []
output_num_clusters = []
output_infile = []
output_obj_func = []
output_entropy = []
output_purity = []
output_time = []

for crit in crit_funcs:
    for infile in ifnames:
        for clusters in num_clusters:
            proc = subprocess.Popen(["./kcluster", infile, crit, classname, str(clusters), str(num_trials), outfile], stdout=subprocess.PIPE)
            output = proc.stdout.readlines()

            #print output
            obj = output[0].split(":")[1].strip()
            ent = output[1].split(":")[1].strip()
            pur = output[2].split(":")[1].strip()
            time = output[3].split(":")[1].strip()

            output_crit_funcs.append(crit)
            output_infile.append(infile[2:])
            output_num_clusters.append(clusters)
            output_obj_func.append(obj)
            output_entropy.append(ent)
            output_purity.append(pur)
            output_time.append(time)
            #print "./kcluster" + " " + infile + " " + crit + " " + classname + " " + str(clusters) + " " + str(num_trials) + " " + outfile + "\n"

for i in range(0,len(output_obj_func)):
    print '{} {} {:d} {} {} {} {}\n'.format(output_crit_funcs[i], output_infile[i], output_num_clusters[i], \
            output_obj_func[i], output_entropy[i], output_purity[i], output_time[i])

