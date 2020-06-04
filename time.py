
from multiprocessing import Process, Manager, Value, Array, Event, Queue
import time

print(0b101 & 0b100 == 0b100)
print(0b101 & 0b010 == 0b010)
print(0b101 & 0b001 == 0b001)

print(0b100 >> 1 == 0b010)

b = 0b1001


print(b)

print("-------")

def hardware(d, a, n, e, i, q):
    time.sleep(5)
    d.value = 2
    a[0] = 1023
    n.li = [9,8,7]
    e.set()
    i[0] = 4
    q.put("test")



def play(d, a, n, e, i, q):
    while True:
        time.sleep(1)
        print(a[:])
        print(d.value)
        print(n.li)
        print(e.is_set())
        print("dct")
        print(i)
        print("queue")
        print(q.get())


e = Event()
m = Manager()
n = m.Namespace()
q = Queue()
n.li = []
i = m.list()
i.append(9)
d = Value("i", 1)
a = Array("i", [512, 512])
h = Process(target=hardware, args=(d, a, n, e, i, q))
h.start()

p = Process(target=play, args=(d, a, n, e, i,q))
p.start()

h.join()
p.join()