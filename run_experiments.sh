#! /bin/bash

resultdir="results"
mkdir -p "$resultdir"

function run {
    # Run with some default settings
    cmd="NS_GLOBAL_VALUE=\"RngRun=$1\" waf --run \"shift ${@:3} --apps=20 --apprate=1Mbps --linkrate=30Mbps --prefix=$resultdir/$2\""
    echo "$cmd"
    eval "$cmd"
    #(cd $resultdir && eval "$cmd")
}

# Repeat ten times
for run in {0..9}; do

# Simulate different traffic mixes
run $run "w1_$run" --w1=1 --w2=0 --w3=0 --congestion=0
run $run "w2_$run" --w1=0 --w2=1 --w3=0 --congestion=0
run $run "w3_$run" --w1=0 --w2=0 --w3=1 --congestion=0

run $run "w1w2_$run" --w1=0.5 --w2=0.5 --w3=0 --congestion=0
run $run "w1w3_$run" --w1=0.5 --w2=0 --w3=0.5 --congestion=0

run $run "skewed1_$run" --w1=0.6 --w2=0.3 --w3=0.1 --congestion=0
run $run "skewed3_$run" --w1=0.1 --w2=0.3 --w3=0.6 --congestion=0

# Also simulate congestion levels
run $run "c1_w1_$run" --w1=1 --w2=0 --w3=0 --congestion=5Mbps
run $run "c2_w1_$run" --w1=1 --w2=0 --w3=0 --congestion=10Mbps
run $run "c3_w1_$run" --w1=1 --w2=0 --w3=0 --congestion=15Mbps

run $run "c1_w2_$run" --w1=0 --w2=1 --w3=0 --congestion=5Mbps
run $run "c2_w2_$run" --w1=0 --w2=1 --w3=0 --congestion=10Mbps
run $run "c3_w2_$run" --w1=0 --w2=1 --w3=0 --congestion=15Mbps


done