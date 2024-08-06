import os
import sys
import re
import csv
import argparse
from pathlib import Path
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='run bench on specific gpu')
    parser.add_argument('-gpu', dest='gpu', type=str, help='GPU ID 0-7.')
    parser.add_argument('-csv', dest='csv', type=str, help='CSV file contains profiling sizes.')
    args = parser.parse_args()
    cwd = os.getcwd()
    #csv_file = os.path.join(cwd, args.csv)
    csv_file = os.path.join(cwd, "rest_gpu"+str(args.gpu)+".csv")
    bench_script = os.path.join(cwd, "run_hipblaslt_bench_heuristic.sh")
#    print(cwd)
#    print(args.gpu)
#    print(csv_file)
#    print(bench_script)

    with open(csv_file, encoding='utf-8-sig', newline='') as f:
        os.environ['HIP_VISIBLE_DEVICES'] = str(args.gpu)
        os.environ['HIPBLASLT_FLUSH'] = "1"
#        os.environ['HIPBLASLT_BENCHMARK'] = "1"
        r = csv.reader(f)

        for lines in r:
            M = lines[0]
            N = lines[1]
            K = lines[2]

            # -1
            subprocess.run([bench_script, str(1), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16', cwd])
#
#            for solnum in range(1,11):
#                subprocess.run([bench_script, str(solnum), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16', cwd])
#            for solnum in range(15, 30, 5):
#                subprocess.run([bench_script, str(solnum), str(int(M)), str(N), str(K), 'T', 'N', 'bf16', 'bf16', 'bf16', cwd])
#
