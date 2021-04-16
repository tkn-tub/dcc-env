#!/bin/bash

set -eux

GRID_SIZE=6
SCENARIO=manhattan
NUM_FLOWS=100

netgenerate --grid --grid.number $GRID_SIZE --no-turnarounds -o $SCENARIO.net.xml

python $SUMO_HOME/tools/randomTrips.py --flows $NUM_FLOWS -b 0 -e 1 -n $SCENARIO.net.xml -o $SCENARIO.flow.xml --jtrrouter --trip-attributes 'departPos="random" departSpeed="max"'

jtrrouter --net-file $SCENARIO.net.xml --route-files $SCENARIO.flow.xml --output-file $SCENARIO.rou.xml --turn-defaults "25,50,25" --accept-all-destinations --allow-loops
