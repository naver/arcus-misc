"""
/*
 * Copyright 2012-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
"""
import time
import sys
import random
import threading
from threading import Lock


# To get memcache.py, go to https://pypi.python.org/pypi/python-memcached/
import memcache

MEMCACHED_SERVER = '127.0.0.1:11211'
# 1440 mins = 24 hours
N_WORKERS = 2
KEY = 'k'

class Setter(threading.Thread):
    def __init__(self, mc):
        threading.Thread.__init__(self)
        self.setDaemon(True)

        self.ncount = 0
        self.mc = mc

    def run(self):
        print 'set start!'

        while(True):
            if self.ncount > 100:
                return

            for i in range(100):
                ret = self.mc.set(KEY, '1')
                if ret is 0:
                    print 'set failed!'
                    return

                ret = self.mc.incr(KEY)
                if ret is 0:
                    print 'incr failed!'
                    return
            self.ncount += 1

def main():
    mc = memcache.Client([MEMCACHED_SERVER], debug=0)

    workers = []

    # creating workers
    for i in range(N_WORKERS):
        worker = Setter(mc)
        workers.append(worker)
    
    for worker in workers:
        worker.start()

    for worker in workers:
        worker.join()

if __name__ == '__main__':
    main()
