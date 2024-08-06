#!/bin/bash

# Usage: gen_pmlog.sh log_file_name "your_workload"
AGT_PATH="/workspace/tool/agt_tool/agt_internal"
CSV_PARSER_PATH="/workspace/tool/pmlog/csv_parser.py"
CLK_CSV_PARSER_PATH="/workspace/tool/pmlog/csv_parser_clk.py"
#echo "Starting..."
#echo "PMLog file name: $1"
FILENAME="$1"
if test -f "$FILENAME"; then
    echo "$FILENAME exists. Remove the file name"
    NOW="$(date +"%Y_%m_%d__%H_%M")"
    FILENAME=$NOW"_"$FILENAME
    echo "$FILENAME"
fi
# MI300A
#sudo <path_to_agt>/agt/agt_internal -i=5 -pmperiod=50 -pmlogall -pmstopcheck -pmnoesckey -pmoutput=$FILENAME 2>&1 &
# MI300X
id=$((5*$2))
DEVICEID="$id"  # DEVICEID="20"  # od * 5

sudo $AGT_PATH -i=$DEVICEID -pmperiod=50 -pmlogall -pmstopcheck -pmnoesckey -pmoutput=$FILENAME 2>&1 &

eval $3
python3 $CSV_PARSER_PATH $FILENAME $DEVICEID
python3 $CLK_CSV_PARSER_PATH $FILENAME $DEVICEID
touch terminate.txt
stty sane

