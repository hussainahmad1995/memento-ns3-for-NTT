#! /bin/bash

resultdir="results"
mkdir -p "$resultdir"

# Build modules once, then run without building.
waf build

function run {
    # Run with some default settings
    cmd="NS_GLOBAL_VALUE=\"RngRun=$1\" waf --run-no-build \"trafficgen ${@:3} --apps=20 --apprate=1Mbps --linkrate=30Mbps --prefix=$resultdir/$2\""
    echo "$cmd"
    eval "$cmd"
    #(cd $resultdir && eval "$cmd")
}

# Repeat 22 times with different RNG to have plenty of data.
for run in {0..30}; do

# Simulate different traffic mixes
run $run "w1_$run" --w1=1 --w2=0 --w3=0 --congestion=0
run $run "w2_$run" --w1=0 --w2=1 --w3=0 --congestion=0
run $run "w3_$run" --w1=0 --w2=0 --w3=1 --congestion=0

run $run "w1w2_$run" --w1=0.5 --w2=0.5 --w3=0 --congestion=0
run $run "w1w3_$run" --w1=0.5 --w2=0 --w3=0.5 --congestion=0

run $run "skewed1_$run" --w1=0.6 --w2=0.3 --w3=0.1 --congestion=0
run $run "skewed3_$run" --w1=0.1 --w2=0.3 --w3=0.6 --congestion=0

# Also simulate congestion levels
# 100% average network utilization
run $run "w1_c100_$run" --w1=1 --w2=0 --w3=0 --congestion=10Mbps
run $run "w2_c100_$run" --w1=0 --w2=1 --w3=0 --congestion=10Mbps
run $run "w3_c100_$run" --w1=0 --w2=0 --w3=1 --congestion=10Mbps

# 150% average network utilization
run $run "w1_c133_$run" --w1=1 --w2=0 --w3=0 --congestion=20Mbps
run $run "w2_c133_$run" --w1=0 --w2=1 --w3=0 --congestion=20Mbps
run $run "w3_c133_$run" --w1=0 --w2=0 --w3=1 --congestion=20Mbps


done