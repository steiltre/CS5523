#!/usr/bin/env python

import subprocess
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

plt.rcdefaults()

ifname = "./large"

min_conf = [0.8, 0.9, 0.95]

min_supp = [30, 50, 100, 500, 1000]
min_supp_no_rule = [15, 20]

num_trials = 10

titlesize=18
labelsize=16

def label_bar_dbl(graph, ylabels):
    max_y = ax.get_ylim()[1]
    i = 0;
    for rect in graph:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., height + 0.005*max_y, "%0.04f" % ylabels[i], ha='center', va='bottom')
        i += 1

def label_bar_int(graph, ylabels):
    max_y = ax.get_ylim()[1]
    i = 0;
    for rect in graph:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., height + 0.005*max_y, "%d" % ylabels[i], ha='center', va='bottom')
        i += 1

freq_itemset_times = {}
freq_itemset_counts = {}
rule_times = {}
rule_counts = {}

for supp in min_supp_no_rule:
    freq_key = supp
    for i in range(0,num_trials):
        proc = subprocess.Popen(["./fptminer", str(supp), "0", ifname], stdout=subprocess.PIPE)
        output = proc.stdout.readlines()
        time = float(output[0].split(":")[1].strip().split()[0])
        if freq_key in freq_itemset_times:
            freq_itemset_times[freq_key] += time/num_trials
        else:
            freq_itemset_times[freq_key] = time/num_trials

        count = int(output[1].split(":")[1].strip().split()[0])
        freq_itemset_counts[freq_key] = count

for supp in min_supp:
  for conf in min_conf:
    freq_key = supp
    conf_key = supp, conf
    for i in range(0,num_trials):
      proc = subprocess.Popen(["./fptminer", str(supp), str(conf), ifname], stdout=subprocess.PIPE)
      output = proc.stdout.readlines()
      time = float(output[0].split(":")[1].strip().split()[0])
      if freq_key in freq_itemset_times:
        freq_itemset_times[freq_key] += time/(num_trials*len(min_conf))
      else:
        freq_itemset_times[freq_key] = time/(num_trials*len(min_conf))

      count = int(output[1].split(":")[1].strip().split()[0])
      freq_itemset_counts[freq_key] = count;

      time = float(output[2].split(":")[1].strip().split()[0])
      if conf_key in rule_times:
        rule_times[conf_key] += time/num_trials
      else:
        rule_times[conf_key] = time/num_trials

      count = int(output[3].split(":")[1].strip().split()[0])
      rule_counts[conf_key] = count

freq_times_plotx = []
freq_times_ploty = []
freq_counts_plotx = []
freq_counts_ploty = []

for supp in min_supp_no_rule:
    freq_times_plotx.append(supp)
    freq_times_ploty.append(freq_itemset_times[supp])
    freq_counts_plotx.append(supp)
    freq_counts_ploty.append(freq_itemset_counts[supp])

for supp in min_supp:
    freq_times_plotx.append(supp)
    freq_times_ploty.append(freq_itemset_times[supp])
    freq_counts_plotx.append(supp)
    freq_counts_ploty.append(freq_itemset_counts[supp])

fig, ax = plt.subplots()
fig.suptitle("Time required for frequent itemset generation", fontsize=titlesize)
ax.set_xlabel("Minimum support count", fontsize=labelsize)
ax.set_ylabel("Time (s)", fontsize=labelsize)
graph = ax.bar(range(len(freq_times_plotx)), freq_times_ploty, tick_label=freq_times_plotx, align='center', alpha = 0.7)
label_bar_dbl(graph, freq_times_ploty)
#plt.show()
plt.savefig("./freq_time.ps")

fig, ax = plt.subplots()
fig.suptitle("Frequent itemsets generated", fontsize=titlesize)
ax.set_xlabel("Minimum support count", fontsize=labelsize)
ax.set_ylabel("Number of frequent itemsets", fontsize=labelsize)
graph = ax.bar(range(len(freq_counts_plotx)), freq_counts_ploty, tick_label=freq_counts_plotx, align='center', alpha=0.7)
label_bar_int(graph, freq_counts_ploty)
#plt.show()
plt.savefig("./freq_count.ps")

rule_times_plotx = []
rule_times_ploty = []
rule_counts_plotx = []
rule_counts_ploty = []
selected_supp = 50

for conf in min_conf:
    rule_times_plotx.append(conf)
    rule_times_ploty.append( rule_times[selected_supp,conf] )
    rule_counts_plotx.append(conf)
    rule_counts_ploty.append( rule_counts[selected_supp,conf] )

fig, ax = plt.subplots()
fig.suptitle("Time required for rule generation", fontsize=titlesize)
ax.set_xlabel("Minimum confidence", fontsize=labelsize)
ax.set_ylabel("Time (s)", fontsize=labelsize)
graph = ax.bar(range(len(rule_times_plotx)), rule_times_ploty, tick_label=rule_times_plotx, align='center', alpha=0.7)
label_bar_dbl(graph, rule_times_ploty)
#plt.show()
plt.savefig("./rule_time.ps")

fig, ax = plt.subplots()
fig.suptitle("Number of rules generated", fontsize=titlesize)
ax.set_xlabel("Minimum confidence", fontsize=labelsize)
ax.set_ylabel("Number of rules", fontsize=labelsize)
graph = ax.bar(range(len(rule_counts_plotx)), rule_counts_ploty, tick_label=rule_counts_plotx, align='center', alpha=0.7)
label_bar_int(graph, rule_counts_ploty)
#plt.show()
plt.savefig("./rule_count.ps")

