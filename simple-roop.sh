#!/bin/bash

NUM=${1}

echo "num,block-order,goodput,time" >> result
for i in `seq 1 ${NUM}`
do
  echo "${i},$(./thread-tcp-para-get)" >> result
done
