import os
import sys
import re
import csv
import argparse
from pathlib import Path
import numpy as np

def parsing_log_and_write_csv(dir_path, output_filename, rsolKey):
    #fieldnames = ['M', 'N', 'K', 'perf', 'splitK', 'Name', 'Solution']
    fieldnames = ['M', 'N', 'K', 'sel-1', 'sel1', 'sel2', 'sel3', 'sel4', 'sel5', 'sel6', 'sel7', 'sel8', 'sel9', 'sel10', 'sel15', 'sel20', 'sel25']
    case_fieldnames = ['M', 'N', 'K', 'GPUID']
    case_M = []
    case_N = []
    case_K = []
    case_gpu = []
    M1 = []
    N1 = []
    K1 = []
    U1 = []  # time in us
    S1 = []  # solution name
    L1 = []  # kernel name
    SK = []  # splitK number
    SEL_U1 = []
    SEL_S1 = []
    SEL_L1 = []
    SEL_SK = []
    max_heuristic = 25
    heuristic_cnt = 13
    case_file_name = os.path.join(dir_path, 'benchmark_case.csv')
    if Path(case_file_name).exists():
        os.remove(case_file_name)

    for filename in os.listdir(dir_path):
        if filename.endswith("sol-1_gpu2.txt") or filename.endswith("sol-1_gpu3.txt") or filename.endswith("sol-1_gpu4.txt") or filename.endswith("sol-1_gpu5.txt") or filename.endswith("sol-1_gpu6.txt") or filename.endswith("sol-1_gpu7.txt"):
            split_str = filename.split('_')
            case_M.append(split_str[1])
            case_N.append(split_str[2])
            case_K.append(split_str[3])
            case_gpu.append(split_str[5])
            #print(split_str)
    with open(case_file_name, 'a') as f:
        csvfile = csv.DictWriter(f, case_fieldnames)
        csvfile.writeheader()
        for i in range(len(case_M)):
            #print( case_M[i], case_N[i], case_K[i], case_gpu[i][3])
            csvfile.writerow({'M': case_M[i], 'N': case_N[i], 'K': case_K[i], 'GPUID': case_gpu[i][3]})

    for filename in os.listdir(dir_path):
        SEL_u = []
        SEL_s = []
        SEL_l = []
        SEL_sk = []
        print("[BRIANNA] ", filename, " ------------------------")
        has_splitK = False
        rsol = '0'
        if filename.endswith(".txt"):
            rsol = filename.split('sol')[1].split('_')[0]
            sel = 0
            if rsol == rsolKey:
                get_winner = False
                with open(os.path.join(dir_path, filename)) as f:
                    lines = f.readlines()
                    for line in lines:
                        if rsol == '1':
                            match_winner = re.match(r'Is supported 1 / Total solutions: 1', line)
                        else:
                            match_winner = re.match(r'Winner:', line)
                        if match_winner != None:
                            get_winner = True

                        r = re.search('splitK', line)
                        if r != None:
                            has_splitK = True
                        result = re.match(r'    T,N,0,1,([0-9]*),([0-9]*),([0-9]*),.*,([0-9]*),' + \
                                          r'[ ]*([\.0-9]*),[ ]*([\.0-9]*)', line)
                        if result != None:
                            sel += 1
                        if result != None and sel <= max_heuristic:
                            SEL_u.append(float(result.group(6)))
                            if has_splitK:
                                SEL_sk.append(result.group(4))
                            else:
                                SEL_sk.append(0)

                        if result != None and get_winner == True:
                            if has_splitK:
                                SK.append(result.group(4))
                            else:
                                SK.append(0)
                            M1.append(result.group(1))
                            N1.append(result.group(2))
                            K1.append(result.group(3))
                            U1.append(result.group(6))
                            #print(result.group(1), result.group(2), result.group(3), result.group(6))

                        get_solution = re.match(r'    --Solution name:  (.*)', line)
                        if get_solution != None and get_winner == True:
                            S1.append(get_solution.group(1).strip())
                        if get_solution != None and sel <= max_heuristic:
                            SEL_s.append(get_solution.group(1).strip())
                        get_kernel = re.match(r'    --kernel name:  (.*)', line)
                        if get_kernel != None and get_winner == True:
                            L1.append(get_kernel.group(1).strip())
                        if get_kernel != None and sel <= max_heuristic:
                            SEL_l.append(get_kernel.group(1).strip())
            for s in range(0, 10):
                heuristic_u = SEL_u[0:s+1]
                #print(heuristic_u)
                #print(type(heuristic_u[0]))
                heuristic_s = SEL_s[0:s+1]
                heuristic_l = SEL_l[0:s+1]
                heuristic_sk = SEL_sk[0:s+1]
                sort_idx = np.argsort(heuristic_u)
                #print(sort_idx)
                #print(heuristic_u[sort_idx[0]])
                SEL_U1.append(heuristic_u[sort_idx[0]])
                SEL_S1.append(heuristic_s[sort_idx[0]])
                SEL_L1.append(heuristic_l[sort_idx[0]])
                SEL_SK.append(heuristic_sk[sort_idx[0]])

            heuristic_u = SEL_u[0:15]
            heuristic_s = SEL_s[0:15]
            heuristic_l = SEL_l[0:15]
            heuristic_sk = SEL_sk[0:15]
            sort_idx = np.argsort(heuristic_u)
            SEL_U1.append(heuristic_u[sort_idx[0]])
            SEL_S1.append(heuristic_s[sort_idx[0]])
            SEL_L1.append(heuristic_l[sort_idx[0]])
            SEL_SK.append(heuristic_sk[sort_idx[0]])

            heuristic_u = SEL_u[0:20]
            heuristic_s = SEL_s[0:20]
            heuristic_l = SEL_l[0:20]
            heuristic_sk = SEL_sk[0:20]
            sort_idx = np.argsort(heuristic_u)
            SEL_U1.append(heuristic_u[sort_idx[0]])
            SEL_S1.append(heuristic_s[sort_idx[0]])
            SEL_L1.append(heuristic_l[sort_idx[0]])
            SEL_SK.append(heuristic_sk[sort_idx[0]])

            heuristic_u = SEL_u[0:25]
            heuristic_s = SEL_s[0:25]
            heuristic_l = SEL_l[0:25]
            heuristic_sk = SEL_sk[0:25]
            sort_idx = np.argsort(heuristic_u)
            SEL_U1.append(heuristic_u[sort_idx[0]])
            SEL_S1.append(heuristic_s[sort_idx[0]])
            SEL_L1.append(heuristic_l[sort_idx[0]])
            SEL_SK.append(heuristic_sk[sort_idx[0]])

        print("[BRIANNA]", SEL_u)
        #print(SEL[0:2][np.argsort(SEL[0:2])[0]])
        #print(SEL_S)
        #print(SEL_K)
    with open(output_filename, 'a') as f:
        csvfile = csv.DictWriter(f, fieldnames)
        csvfile.writeheader()
        for i in range(len(M1)):
            #csvfile.writerow({'M': M1[i], 'N': N1[i], 'K': K1[i], 'perf': U1[i], 'splitK': SK[i], \
            #                 'Name': L1[i], 'Solution': S1[i]})
            csvfile.writerow({'M': M1[i], 'N': N1[i], 'K': K1[i], 'sel-1': U1[i], \
                              'sel1': SEL_U1[heuristic_cnt * i], \
                              'sel2': SEL_U1[heuristic_cnt * i + 1], \
                              'sel3': SEL_U1[heuristic_cnt * i + 2], \
                              'sel4': SEL_U1[heuristic_cnt * i + 3], \
                              'sel5': SEL_U1[heuristic_cnt * i + 4], \
                              'sel6': SEL_U1[heuristic_cnt * i + 5], \
                              'sel7': SEL_U1[heuristic_cnt * i + 6], \
                              'sel8': SEL_U1[heuristic_cnt * i + 7], \
                              'sel9': SEL_U1[heuristic_cnt * i + 8], \
                              'sel10': SEL_U1[heuristic_cnt * i + 9], \
                              'sel15': SEL_U1[heuristic_cnt * i + 10], \
                              'sel20': SEL_U1[heuristic_cnt * i + 11], \
                              'sel25': SEL_U1[heuristic_cnt * i + 12]})
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate performance reports from log (.txt)')
    parser.add_argument('-d', dest='log_directory', type=str, nargs='*',
                        help='Log directory. Can be more than one.')
    parser.add_argument('-o', dest='output_file_name', type=str,
                        help='Report name (.csv). The destination is the same as log_directory.')
    parser.add_argument('-r', dest='requested_num', type=str,
                        help='rsol')
    args = parser.parse_args()

    for d in args.log_directory:
        if Path(d).exists():
            out_csv_name = os.path.join(d, args.output_file_name)
            if Path(out_csv_name).exists():
                os.remove(out_csv_name)
            parsing_log_and_write_csv(d, out_csv_name, args.requested_num)
            #print(out_csv_name, "is done.")
        else:
            print("Error:", d ,"doesn't exist.")

