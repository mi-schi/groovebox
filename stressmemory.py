#!/usr/bin/env python

from multiprocessing import Pool
from multiprocessing import cpu_count
import time


def f(x):
    while True:
        x * x


processes = cpu_count() * 2
print('Utilizing %d cores at %s' % (processes, time.strftime("%Y-%m-%d %H:%M:%S")))
pool = Pool(processes)
pool.map_async(f, range(processes))

while True:
    time.sleep(30)
    print('-' * 20)
    print('Running full CPU load twice at %s' % time.strftime("%Y-%m-%d %H:%M:%S"))
