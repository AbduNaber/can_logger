for i in {1..1000000}
do
    cansend can0 001#0100000000000000
    echo "Sent message 123#0000000000000$((i%10)) - Iteration $i"

done