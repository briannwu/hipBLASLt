#!/bin/bash
sol=(-1)
for solnum in ${sol[@]};
do
    python /workspace/tool/report_parser_compareall.py -d /workspace/hipBLASLt/heuristic_phantom/large/ -o phantom_sel$solnum.csv -r $solnum
done
