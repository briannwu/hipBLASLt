#!/bin/bash

export HIP_VISIBLE_DEVICES=$1
echo $HIP_VISIBLE_DEVICES
export HIPBLASLT_FLUSH=$2

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

sol=(-1 1 2 3 4 5 6 10 15 20 25)
for i in $(seq 1 100);
do
    choose_case
    echo $m $n $k
    for solnum in ${sol[@]};
    do
	/workspace/hipBLASLt/phantom/script/run_hipblaslt_bench_nodevice.sh $solnum $m $n $k T N bf16 bf16 bf16 | tee /workspace/hipBLASLt/phantom/log/gpu$1.txt
    done
done





