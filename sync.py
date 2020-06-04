import time
from multiprocessing import Process, Manager
from typing import List

class Recorder:
    __record = True

    def run(self, record):
        r = record[0]
        p = Process(target=Rec.run2, args=(r, ))
        p.start()
        p.join()

class Rec:
    def run2(self, power):
        while True:
            print(power)
            time.sleep(1)

class Hardware:
    def run(self, records):
        time.sleep(2)
        records[0] = 0


hardware = Hardware()
recorder = Recorder()

manager = Manager()
records = manager.list([1, 0])

p1 = Process(target=hardware.run, args=(records, ))
p1.start()

p2 = Process(target=recorder.run, args=(records, ))
p2.start()





p1.join()
p2.join()