#!/bin/bash
mkdir ./ret_graphics
INPUT_DATA="./acc_input_50sec"

echo "Graphics TO East"
./graphics_client.py -i ${INPUT_DATA} -s 23.21.103.194 > ./ret_graphics/g_east
sleep 5

echo "Graphics TO West"
./graphics_client.py -i ${INPUT_DATA} -s 184.169.142.70 > ./ret_graphics/g_west
sleep 5

echo "Graphics TO EU"
./graphics_client.py -i ${INPUT_DATA} -s 176.34.100.63 > ./ret_graphics/g_eu
sleep 5

echo "Graphics TO ASIA"
./graphics_client.py -i ${INPUT_DATA} -s 46.137.209.173 > ./ret_graphics/g_asia
sleep 5
