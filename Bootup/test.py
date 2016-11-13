import os
import subprocess
import time

#subprocess.call("/root/test/loadPRU", shell=True)
#subprocess.call("/root/test/test", shell=True)

start = time.time()
while time.time() - start < 600:
    with open('/root/test/text.txt', 'w+') as fid:
        fid.write(str(time.time()-start))
    time.sleep(5)
