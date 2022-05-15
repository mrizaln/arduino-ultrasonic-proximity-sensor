#!/bin/env bash

lokasi=$(pwd)
port=$(ls /dev/ttyACM* 2> /dev/null)

if [[ $port = "" ]]; then
    echo no device found
    exit 1
fi

if [[ "$1" == "upload" ]]; then
    arduino --upload prak_8_sensor_dan_transduser.ino --port $port
fi

stty 9600 -F $port raw -echo
kitty -- bash -c "echo \"$lokasi/hasil\"; echo; cat $port | tee \"$lokasi/hasil\"" &

echo ' ' > hasil

flag=0
while [[ $flag -lt 5 ]]; do
    flag=$(cat hasil | wc -l)
    echo $flag
    sleep 1
done

./plot.sh 
