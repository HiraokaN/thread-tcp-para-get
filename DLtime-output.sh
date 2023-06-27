#!/bin/bash

NUM=${1}

if [ ${NUM} -eq 1 ]; then
  echo "${NUM} $(./thread-tcp-para-get)" > DLtime.txt
else
  echo "${NUM} $(./thread-tcp-para-get)" >> DLtime.txt
fi
