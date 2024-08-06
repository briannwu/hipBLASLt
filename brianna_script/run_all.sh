#!/bin/bash

export HIP_VISIBLE_DEVICES=$1
echo $HIP_VISIBLE_DEVICES
#export HIPBLASLT_FLUSH=$2

select_random() {
    printf "%s\0" "$@" | shuf -z -n1 | tr -d '\0'
}

choose_case () {
    m=$((8 + $RANDOM % 57))
    m=$[m*128]
  
    n_arr=(1 2 4 8 16 32 64 128 8192 16384 32768)
    n=$(select_random "${n_arr[@]}")

    k=$((8 + $RANDOM % 57))
    k=$[k*128]
}

m=3584
k=8192
for n in $(seq 1 16 256);
do
    echo $m $n $k
    HIPBLASLT_FLUSH=1 /workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_all_flush.sh 0 $m $n $k T N bf16 bf16 bf16 | tee /workspace/hipBLASLt/phantom/log/N1_256/log_$m_$n_$k.txt

done





