import os
import sys
import re
import csv
import argparse
from pathlib import Path

def parsing_log_and_write_csv(dir_path, output_filename, heuristic1):
    fieldnames = ['M', 'N', 'K', 'gflops', 'perf', 'splitK', 'Name', 'Solution']
    M1 = []
    N1 = []
    K1 = []
    U1 = []  # time in us
    S1 = []  # solution name
    L1 = []  # kernel name
    SK = []  # splitK number
    GF = []

    for filename in os.listdir(dir_path):
        has_splitK = False

        if filename.endswith(".txt"):
            get_winner = heuristic1  # heuristic1 is True -> True; heuristic1 is False -> False
            print(filename, get_winner)
            with open(os.path.join(dir_path, filename)) as f:
                lines = f.readlines()
                for line in lines:
                    match_winner = re.match(r'Winner:', line)
                    if match_winner != None:
                        get_winner = True
                    r = re.search('splitK', line)
                    if r != None:
                        has_splitK = True
                    result = re.match(r'    T,N,0,1,([0-9]*),([0-9]*),([0-9]*),.*,([0-9]*),' + \
                                      r'[ ]*([\.0-9]*),[ ]*([\.0-9]*)', line)
                    if result != None and get_winner == True:
                        if has_splitK:
                            SK.append(result.group(4))
                        else:
                            SK.append(0)
                        M1.append(result.group(1))
                        N1.append(result.group(2))
                        K1.append(result.group(3))
                        GF.append(result.group(5))
                        U1.append(result.group(6))
#                        print(result.group(6))

                    get_solution = re.match(r'    --Solution name:  (.*)', line)
                    if get_solution != None and get_winner == True:
                        S1.append(get_solution.group(1).strip())

                    get_kernel = re.match(r'    --kernel name:  (.*)', line)
                    if get_kernel != None and get_winner == True:
                        L1.append(get_kernel.group(1).strip())
    with open(output_filename, 'a') as f:
        csvfile = csv.DictWriter(f, fieldnames)
        csvfile.writeheader()
        print("len(M1) = ", len(M1))
        for i in range(len(M1)):
            csvfile.writerow({'M': M1[i], 'N': N1[i], 'K': K1[i], 'gflops': GF[i], 'perf': U1[i], 'splitK': SK[i], \
                             'Name': L1[i], 'Solution': S1[i]})
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate performance reports from log (.txt)')
    parser.add_argument('-d', dest='log_directory', type=str, nargs='*',
                        help='Log directory. Can be more than one.')
    parser.add_argument('-o', dest='output_file_name', type=str,
                        help='Report name (.csv). The destination is the same as log_directory.')
    parser.add_argument('-heu1', dest='heuristic1_flag', default=False, action=argparse.BooleanOptionalAction,
                        help='If requested num is 1, set True, otherwise set False.')



    args = parser.parse_args()
    print(args.heuristic1_flag)
    for d in args.log_directory:
        if Path(d).exists():
            out_csv_name = os.path.join(d, args.output_file_name)
            if Path(out_csv_name).exists():
                os.remove(out_csv_name)
            parsing_log_and_write_csv(d, out_csv_name, args.heuristic1_flag)
            print(out_csv_name, "is done.")
        else:
            print("Error:", d ,"doesn't exist.")

