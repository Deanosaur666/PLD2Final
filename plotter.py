import matplotlib.pyplot as plt
import numpy as np
import re

books = [ "Hound", "Frankenstein", "Dracula" ]
bookColors = {
    "Hound" : [(.2, .2, 1), (.1, .1, 0.5)],
    "Frankenstein" : [(.2, 1, .2), (.1, 0.5, .1)],
    "Dracula" : [(1, .2, .2), (0.5, .1, .1)],
}
times = {}
avgTime = {}
expectedtime = {}
maxp = 0

for book in books:
    times[book] = {}
    avgTime[book] = []
    expectedtime[book] = []
    with open("./log_%s.txt" % book, "r") as file:
        p = 0
        for line in file:
            if line[0] == "P":
                p = int(re.search(r"(\d+)", line).group(1))
                times[book][p] = []
                maxp = max(maxp, p)
            elif line[0:2] == "T:":
                t = float(re.search(r"(\d*\.\d+)", line).group(1))
                times[book][p].append(t)

proc = []
for p in range(1, maxp+1):
    proc.append(p)
    for book in books:
        avgTime[book].append((sum(times[book][p]) / len(times[book][p])) * 1000.0) # convert seconds to milliseconds
for i in range(len(proc)):
    for book in books:
        expectedtime[book].append(avgTime[book][0]/(i+1))

for book in books:
    plt.plot(proc, avgTime[book], marker=".", linestyle="-", color=bookColors[book][0], label="%s Real time" % book)
    plt.plot(proc, expectedtime[book], marker=".", linestyle="--", color=bookColors[book][1], label="%s Expected time" % book)

plt.grid(True)
plt.xlabel("Processors")
plt.ylabel("Time (ms)")
plt.legend()
plt.show()
