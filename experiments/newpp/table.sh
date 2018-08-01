#!/bin/bash
# Asssume this file is in a subfolder of experiments/ named newpp/, for instance
# newfolder has a subfolder bench/ containing all benchmarks

OPTS="--mem 128M --stack 6M"
PDPU="./../../dist/bin/dpu"

# remove old log and csv files
rm -Rf logs
mkdir logs
rm pdpu.csv

# for all files .c in bench, run pdpu and push the output into corresponding log files
R="bench"
for bm in $R/*.c; do # iterate all benchmarks in bench/
echo "Produce file text for benchmark $bm"
NAME=${bm:6:4}
for k in 0 1; do # k in 0..5
for core in 2 4; do # core in 2 4 6 8 10 12
SUM=0.0
for iter in 1 2; do  # iter in 1..5
CMD="$PDPU $bm -c$core -k$k $OPTS"
LOG="./logs/${NAME}_core${core}_alt${k}_i${iter}.txt"
echo "$LOG"
$CMD >${LOG}
WALLTIME=$(grep "pdpu: summary" ${LOG}| awk '{print $5}')
# "$WALLTIME"
# compute the sum of walltime for iter times of executions
#SUM=$(printf '%.3f\n' "$( bc <<<"$SUM + $WALLTIME" )")
#echo $SUM
done #done for i
# compute average walltime
#AVG=$(echo "scale=3;$SUM/$iter" | bc)
#echo "Average time: $AVG"
done #done for core
done #done for k
done #done for bm

# Summarize infos in txt files to write in csv file
HEADER="Log file, nr of cores, execution time"
echo "$HEADER" > pdpu.csv
for fl in logs/*.txt; do
    echo $fl >> pdpu.csv ;
    grep "pdpu: summary" $fl| awk '{print $3 "," $5}' >> pdpu.csv
done




