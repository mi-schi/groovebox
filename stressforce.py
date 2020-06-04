#!/usr/bin/env python

from multiprocessing import Pool
from multiprocessing import cpu_count
from random import randint
import time


def f(x):
    while True:
        x * x


while True:
    processes = cpu_count()
    print('-' * 20)
    print('Utilizing %d cores at %s' % (processes, time.strftime("%Y-%m-%d %H:%M:%S")))
    pool = Pool(processes)
    pool.map_async(f, range(processes))
    load_time = randint(1, 10)
    print('Running load on CPU(s) for %d seconds' % load_time)
    time.sleep(load_time)
    pool.close()
    pool.terminate()
    wait_time = randint(1, 10)
    print('Wait for %d seconds' % wait_time)
    time.sleep(wait_time)
