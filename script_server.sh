#!/bin/bash

#./dataServer -s 1 -q 4 -p 3060
./dataServer -s 3 -q 1 -p 3060 &

serverPID=$!

sleep 5

kill -s 15  $serverPID
