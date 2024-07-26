#!/bin/bash

echo "Starting first loop"

for i in {1..1000}
do
    cansend vcan0 123#0000000000000000
    echo "Sent message 123#0000000000000000 - Iteration $i"

done

#echo "Sending specific message"
cansend vcan0 001#0100000000000000


echo "Starting second loop"

for i in {1..10000000}
do
    cansend vcan0 123#1111111111111111
    echo "Sent message 123#1111111111111111 - Iteration $i"

done

echo "Script finished"
