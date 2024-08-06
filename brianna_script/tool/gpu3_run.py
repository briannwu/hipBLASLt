import os
import sys
import re
import csv
import argparse
from pathlib import Path
import subprocess

with open('/workspace/gpu3.csv', encoding='utf-8-sig', newline='') as f:
    os.environ['HIP_VISIBLE_DEVICES'] = "3"
    os.environ['HIPBLASLT_FLUSH'] = "1"
    os.environ['HIPBLASLT_BENCHMARK'] = "1"
    r = csv.reader(f)
 #   csvFile = csv.reader(file)
    for lines in r:
        M = lines[0]
        N = lines[1]
        K = lines[2]
        for solnum in range(1,11):
            subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_step128.sh', str(solnum), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])

        # -1
        subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_step128.sh', str(-1), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])

        for solnum in range(15, 30, 5):
            subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_step128.sh', str(solnum), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])
'''
        # all
        subprocess.run(['/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_getall.sh',str(0),  str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16'])
'''
