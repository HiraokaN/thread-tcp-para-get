#!/bin/bash
FILE_NAME=$1

if [ $# -ne 1 ]; then
  echo "useage ./byte-count FILE NAME"
  exit 0
fi

bash pickup-length.sh ${FILE_NAME} > ${FILE_NAME}-len
./byte-count ${FILE_NAME}-len 
rm ${FILE_NAME}-len

