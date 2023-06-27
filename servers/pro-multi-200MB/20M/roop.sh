#!/bin/bash

#NUMBER=${1}

for i in 1 2 4 6 8 10 12 14 16 18 20
do
  sed -r "s/20MB/200MB/" ${i} > tmp
  mv tmp ${i} 
done
