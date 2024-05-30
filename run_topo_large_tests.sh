#!/bin/bash

# Original author: Siddhant Ray

## First in arguments is the topology
## Second specifies different congestion for different receivers (default is 0)
## Third specifies the seed for the random number generator (change for multiple runs)

## Current setup generates fine-tuning data with 2 bottlenecks, $2 is the second bottleneck rate (!=0)
## To generate pre-training data with only one bottleneck, replace --prefix with 
## --prefix=results/small_test_no_disturbance_with_message_ids$3 and pass $2 as 0

#    // Network topology 1
#    //
#    //                                  disturbance1
#    //                                       |
#    // 3x n_apps(senders) --- switchA --- switchB --- receiver1
#    // 

# If running inside the VSCode's environment to run Docker containers: Replace ./docker-run.sh waf with just waf 

mkdir -p results_test
./docker-run.sh waf --run "trafficgen_large_tests
                    --topo=$1
                    --apps=1
                    --apprate=1Mbps
                    --startwindow=5
                    --queuesize=100p
                    --linkrate=5Mbps
                    --linkdelay=5ms
                    --w1=1
                    --w2=1
                    --w3=1
                    --cc=ns3::TcpCubic
                    --congestion1=$2Mbps
                    --congestion2=$3Mbps
                    --congestion3=$4Mbps
                    --prefix=results_test_large/small_test_no_disturbance_with_message_ids$5
                    --seed=$5"

