#!/bin/bash

#if [ ${#} -ne 1 ]; then
#  echo "how to use 'bash auto-increase.sh {# of execute}'"
#  exit 0
#fi

#NUM=${1}
#LAST=${2}
#DEV=${3}

for i in 2 4
do
  cp servers/pro-multi-200MB/20M/${i} tcp-para-get-servers.h
  make
  for j in `seq 1 10`
  do
    echo "${j} $(./thread-tcp-para-get)" >> ../gnuplot/multi-path/10M10ms1%,10M10ms1%/gather/eval-${i} 
  done
done
