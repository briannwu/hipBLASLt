#!/bin/bash

echo $HIP_VISIBLE_DEVICES
export HIPBLASLT_BIN='/workspace/hipBLASLt/build/release/clients/staging/hipblaslt-bench'

usagestr="run_hipblaslt_bench.sh <sol> <m> <n> <k> <N/T> <N/T> <f16/f8> <f16/f8> <f32> <optional:splitk1> <optional:splitk2>\nex. ./run_hipblaslt_bench.sh 128 128 128 N N f16 f16\nex. ./run_hipblaslt_bench.sh 128 128 128 N N f16 f16 4 8"

a_type="$7_r"
b_type="$8_r"
cd_type="$9_r"
compute_type=f32_r

if [[ "$7" = "f8" && "$8" = "f16" ]];
then
    compute_type=f32_f16_r
elif [[ "$7" = "f16" && "$8" = "f8" ]];
then
    compute_type=f32_f16_r
elif [[ "$7" = "f16" && "$8" = "f16" ]];
then
    compute_type=f32_r
elif [[ "$7" = "bf16" && "$8" = "bf16" ]];
then
    compute_type=f32_r
else
    echo "Invalid precision"
    echo -e $usagestr
    exit 1
fi

rotating=''
log_dir='cuda_test'

if [[ $HIPBLASLT_FLUSH = 1 ]];
then
    echo "Rotating buffer enabled"
    rotating=" --rotating 512"
    log_dir='rotate512'
fi

logname='/workspace/hipBLASLt/phantom/log/'$log_dir'/result_'$2'_'$3'_'$4'_sol'$1'_gpu'$HIP_VISIBLE_DEVICES'.txt'
splitkcmd=''
for splitk in ${@:10}
do
    if ! [[ $splitk =~ '^[0-10]+$' ]];
    then
        splitkcmd="$splitkcmd --splitk $splitk"
    fi
done
set -x
export HIP_FORCE_DEV_KERNARG=1
echo $logname
$HIPBLASLT_BIN --a_type $a_type \
               --b_type $b_type \
               --c_type $cd_type \
               --d_type $cd_type \
               --compute_type $compute_type \
               --transA $5 \
               --transB $6 \
               --api_method cpp \
               --requested_solution $1 \
               -j 100 -i 100 \
	       --initialization trig_float \
               --print_kernel_info \
               -m $2 -n $3 -k $4 \
               $rotating --splitk 0 $splitkcmd > $logname
set +x

