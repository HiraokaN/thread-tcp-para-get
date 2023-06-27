#!/bin/bash
FILE_NAME=$1
#count=0
#cnt=0

if [ $# -ne 1 ]; then
  echo "useage ./extract FILE NAME"
  exit 0
fi

while read LINE
do
  if [[ $LINE == *"HTTP"* ]]
  then
    echo ${LINE} | sed -e 's/.*\([0-9]\{2\}\):\([0-9]\{2\}\):\([0-9]\{2\}\).\([0-9]\{6\}\).\(ens[0-9]\).*IP.192.168.[0-9]0.[0-9].[0-9]*.>.192.168.[0-9]0.[0-9].\([0-9]*\).*length.\([0-9]*\).*/\1 \2 \3 \4 \5 \6 \7/'
  fi
done < ${FILE_NAME}

#echo ${count}
#echo ${cnt}
#count=`expr ${count} + 1`
#if [[ $LINE == *"HTTP/1.1 206 Partial Content"* ]]
