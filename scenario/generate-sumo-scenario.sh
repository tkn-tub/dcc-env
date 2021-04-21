#!/bin/bash

set -eux

GRID_SIZE=6
GRID_LENGTH=100
BUILDING_OFFSET=10
SCENARIO=manhattan
NUM_FLOWS=100

netgenerate --grid --grid.number $GRID_SIZE --grid.length $GRID_LENGTH --no-turnarounds -o $SCENARIO.net.xml

python $SUMO_HOME/tools/randomTrips.py --flows $NUM_FLOWS -b 0 -e 1 -n $SCENARIO.net.xml -o $SCENARIO.flow.xml --jtrrouter --trip-attributes 'departPos="random" departSpeed="max"'

jtrrouter --net-file $SCENARIO.net.xml --route-files $SCENARIO.flow.xml --output-file $SCENARIO.rou.xml --turn-defaults "25,50,25" --accept-all-destinations --allow-loops

# generate buildings polygon file
set +x

echo '<?xml version="1.0" encoding="utf-8"?>' > $SCENARIO.poly.xml
echo '<shapes>' >> $SCENARIO.poly.xml

for row in $(seq 0 $(($GRID_SIZE - 2))); do
    for col in $(seq 0 $(($GRID_SIZE - 2))); do
        let xmin=$row*$GRID_LENGTH+$BUILDING_OFFSET
        let xmax=($row+1)*$GRID_LENGTH-$BUILDING_OFFSET
        let ymin=$col*$GRID_LENGTH+$BUILDING_OFFSET
        let ymax=($col+1)*$GRID_LENGTH-$BUILDING_OFFSET
        echo "    <poly id=\"building-$row-$col\" type=\"building\" color=\"1.00,0.00,0.00\" fill=\"1\" layer=\"1\" shape=\" $xmin,$ymin $xmin,$ymax $xmax,$ymax $xmax,$ymin $xmin,$ymin\"/>" >> $SCENARIO.poly.xml
        # echo "Building $row-$col: ($xmin, $ymin) to ($xmax, $ymax)"
    done
done

echo '</shapes>' >> $SCENARIO.poly.xml
