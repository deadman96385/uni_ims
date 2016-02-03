#!/user/bin/env python3
import string
import numpy as np
import matplotlib.pyplot as plt

speed_file = open('speed_M.log','r')
index = len('01-03 01:39:40.305   242   242 I ylog    : [01-03 01:39:40.305] ylog<debug> ydst write speed ')
index_str = 0
x = []
y = []
while True:
	line = speed_file.readline()
	if len(line) == 0:
		break
	speed_str = line[index:]
	if speed_str.find('K/s') != -1:
		index_str = speed_str.find('K/s')
		speed = string.atof(speed_str[:index_str])
	else:
		index_str = speed_str.find('M/s')
		speed = string.atof(speed_str[:index_str]) * 1024
	x.append(speed)
	# print(speed)
speed_file.close()

top_file = open('top.log','r')

while True:
	line = top_file.readline()
	if len(line) == 0:
		break
	if line.find('% S') != -1:
		index_str_b = line.find('% S')
	usage = string.atof(line[index_str_b-3:index_str_b])
	y.append(usage)
	# print(usage)
top_file.close()

print(len(x))
print(len(y))

# labels = ['Frogs', 'Hogs', 'Bogs', 'Slogs']
plt.plot(x, y, 'ro')
# You can specify a rotation for the tick labels in degrees or with keywords.
# plt.xticks(x, labels, rotation='vertical')
# Pad margins so that markers don't get clipped by the axes
# plt.margins(0.1)
# Tweak spacing to prevent clipping of tick-labels
plt.xlabel('log speed KB/s')
plt.ylabel('cpu usage %')
plt.title(r'data = 1024B')
plt.subplots_adjust(bottom=0.15)
plt.show()