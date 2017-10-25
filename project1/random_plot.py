import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

plt.rcdefaults()

def label_bar_int(graph, ylabels):
    max_y = ax.get_ylim()[1]
    i = 0;
    for rect in graph:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., height + 0.005*max_y, "%d" % ylabels[i], ha='center', va='bottom')
        i += 1

plotx = [0.8, 0.9, 0.95]
ploty = [618734, 380708, 234094]

fig, ax = plt.subplots()
fig.suptitle("Number of candidate rules generated", fontsize=18)
ax.set_xlabel("Minimum confidence", fontsize=16)
ax.set_ylabel("Number of candidate rules", fontsize=16)
graph = ax.bar(range(len(plotx)), ploty, tick_label=plotx, align='center', alpha=0.7)

label_bar_int(graph, ploty)
#plt.show()
plt.savefig("./cand_rule.pdf")

