#! /bin/bash

function run {
    # Run with some default settings
    cmd="NS_GLOBAL_VALUE=\"RngRun=$1\" waf --run \"shift ${@:2} --apps=20 --apprate=1Mbps --linkrate=30Mbps\""
    echo $cmd
    eval "$cmd"
}

# Repeat ten times
for run in {0..9}; do

# Simulate different traffic mixes
run $run --prefix="w1_$run" --w1=1 --w2=0 --w3=0 --congestion=0
run $run --prefix="w2_$run" --w1=0 --w2=1 --w3=0 --congestion=0
run $run --prefix="w3_$run" --w1=0 --w2=0 --w3=1 --congestion=0

run $run --prefix="w1w2_$run" --w1=0.5 --w2=0.5 --w3=0 --congestion=0
run $run --prefix="w1w3_$run" --w1=0.5 --w2=0 --w3=0.5 --congestion=0

run $run --prefix="skewed1_$run" --w1=0.6 --w2=0.3 --w3=0.1 --congestion=0
run $run --prefix="skewed3_$run" --w1=0.1 --w2=0.3 --w3=0.6 --congestion=0

# Also simulate congestion levels
run $run --prefix="c1_$run" --w1=1 --w2=0 --w3=0 --congestion=1Mbps
run $run --prefix="c2_$run" --w1=1 --w2=0 --w3=0 --congestion=2Mbps
run $run --prefix="c3_$run" --w1=1 --w2=0 --w3=0 --congestion=3Mbps

done