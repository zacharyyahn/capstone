#!/usr/bin/python3
import matplotlib.pyplot as plt

with open("timing.txt") as f:
    times = [(float(line)) / 1000000 for line in f]

print(max(times))
print(sum(times) / len(times))

counts = plt.hist(times, bins=list(range(0, 10)))[0]
print((len(times) - (counts[-1] + counts[-2] + counts[-3])) / len(times))
plt.xlim(0, 9)
plt.xlabel("Image processing time (ms)")
plt.ylabel("Number of frames")
plt.title(f"Image processing time distribution over {len(times)} frames")
plt.show()
