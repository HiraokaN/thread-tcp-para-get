#!/bin/bash

if [ ${#} -ne 2 ]; then
  echo "how to use 'bash auto-all.sh {# of execute} {path_setting(bandwidth[M]delay[ms]loss[%],bandwidth[M]delay[ms]loss[%]}"
  exit 0
fi

NUM=${1}
PATH_CONFIG=${2}

division=(50 100 200 500 1000 2000 5000 10000)

for i in "${division[@]}"
do
  sed -i -e "s/divisions=[0-9]\+/divisions=${i}/" tcp-para-get-pipe.h 
  make
  for j in `seq 1 ${NUM}` 
  do 
    if [ ${j} -eq 1 ]; then
      echo "num,block-order,goodput,time" >> /home/client/study/progressive/gnuplot/multi-path/${PATH_CONFIG}/division/${i}BL
    fi 
    echo "${j},$(./thread-tcp-para-get)" >> /home/client/study/progressive/gnuplot/multi-path/${PATH_CONFIG}/division/${i}BL 
  done
  echo "***${i}BL-result***" >> /home/client/study/progressive/gnuplot/multi-path/${PATH_CONFIG}/division/all_result
  python3 static.py /home/client/study/progressive/gnuplot/multi-path/${PATH_CONFIG}/division/${i}BL >> /home/client/study/progressive/gnuplot/multi-path/${PATH_CONFIG}/division/all_result
done
