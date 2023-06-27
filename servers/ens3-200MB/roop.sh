#!/bin/bash

#NUMBER=${1}

for i in `seq 1 20`
do
  sed -r "s/20MB/200MB/" servers${i}.h > tmp
  mv tmp servers${i}.h 
done
