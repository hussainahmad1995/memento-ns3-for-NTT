#! /bin/sh

for interval in 0.01 0.05 0.1 0.5 1; do
#for interval in 0.01 0.1; do
    for burstsize in 4; do
        echo "Burstsize: $burstsize, Interval: $interval"
        waf --run "probing_test --interval=$interval --burstsize=$burstsize"
    done
done
