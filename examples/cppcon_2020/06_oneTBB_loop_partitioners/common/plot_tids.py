#==========================================
# Copyright © 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

import sys
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.colors import BoundaryNorm
from matplotlib.ticker import MaxNLocator

input_file = "partitioner.csv"

num_values_per_thread = 0 
num_threads = 0 
partitioner = ''
execution_time = ''
num_body_tasks = ''
with open(input_file, 'r') as f:
 a = f.readline().split(',')
 partitioner = a[0]
 num_threads = int(a[1])
 num_values_per_thread = int(a[2])
 execution_time = a[3]
 num_body_tasks = a[4]

Z = np.genfromtxt(input_file,delimiter=',',skip_header=1)
x = np.arange(0, num_values_per_thread + 1, 1)
y = np.arange(0, num_threads + 1, 1)

fig, ax = plt.subplots()
ax.set_title(partitioner + '(' + execution_time + ' seconds with ' + num_body_tasks + ' body tasks)')
ax.pcolormesh(x, y, Z, cmap='tab20b')
