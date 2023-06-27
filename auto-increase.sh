#!/bin/bash

if [ ${#} -ne 3 ]; then
  echo "how to use 'bash auto-increase.sh {first tcp-flow number} {last tcp-flow number} {use interface}'"
  exit 0
fi

FIRST=${1}
LAST=${2}
DEV=${3}

for i in `seq ${FIRST} ${LAST}`
do
  cp servers/${DEV}-200MB/servers${i}.h tcp-para-get-servers.h
  make
  bash DLtime-output.sh ${i} 
done
