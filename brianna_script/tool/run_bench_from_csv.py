import os
import sys
import re
import csv
import argparse
from pathlib import Path
import subprocess

with open('/workspace/GPU2_step128.csv', encoding='utf-8-sig', newline='') as f:
    os.environ['HIP_VISIBLE_DEVICES'] = "2"
    r = csv.reader(f)
 #   csvFile = csv.reader(file)
    for lines in r:
        M = lines[0]
        N = lines[1]
        K = lines[2]
'''
        # -1
        subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench.sh', str(-1), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])
        subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench.sh', str(1), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])
'''
        for solnum in range(7,10):
            # cmd = '/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench.sh ' + str(solnum) + ' ' + str(int(M)) + ' ' + str(N) + ' ' + str(K) + ' T N bf16 bf16 bf16'
            # print(cmd)
            subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_nodevice.sh', str(solnum), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])
'''
        for solnum in range(15, 30, 5):
            subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench.sh', str(solnum), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])

        # all
        subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_getall.sh',str(0),  str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])
'''
