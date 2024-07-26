for i in {1..1000000}
do
    cansend can0 120#111111111111111$((i%10))
    echo "Sent message 120#111111111111111$((i%10)) - Iteration $i"

done
