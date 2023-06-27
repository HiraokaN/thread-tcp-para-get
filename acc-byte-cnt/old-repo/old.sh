#!/bin/bash
FILE_NAME=$1

if [ $# -ne 1 ]; then
  echo "useage ./div-flow FILE NAME"
  exit 0
fi

while read LINE
do
  if [[ $LINE == *"ens3"* ]]
  then
    FLOW=`echo ${LINE} | sed -e 's/[0-9]*.[0-9]*.[0-9]*.ens[0-9].\([0-9]\)/\1/'`
    BUF=`echo ${LINE} | sed -e 's/\([0-9]*.[0-9]*\).\([0-9]*\).\(ens[0-9]\).\([0-9]\)/\1 \2/'`
    echo ${BUF} >> result/ens3-${FLOW}
  fi
  if [[ $LINE == *"ens4"* ]]
  then
    FLOW=`echo ${LINE} | sed -e 's/[0-9]*.[0-9]*.[0-9]*.ens[0-9].\([0-9]\)/\1/'`
    BUF=`echo ${LINE} | sed -e 's/\([0-9]*.[0-9]*\).\([0-9]*\).\(ens[0-9]\).\([0-9]\)/\1 \2/'`
    echo ${BUF} >> result/ens4-${FLOW}
  fi
done < ${FILE_NAME}

