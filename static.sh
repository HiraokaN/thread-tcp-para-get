#! bin/bash
FILE_NAME1=$1
#FILE_NAME2=$2
#CON_NUM=$3
MAX=0
MIN=10000
TOTAL=0
CNT=0

#printf("AVR_OUT_OF_ORDER_ARRIVALS DLtime Goodput\n");

if [ $# -ne 1 ]; then
  echo "useage ./static FILENAME"
  exit 0
fi

while read LINE
do
  AVR_OOO_ARRIVALS=`echo ${LINE} | cut -d " " -f 2`
  if [ `echo "${AVR_OOO_ARRIVALS} > ${MAX}" | bc` == 1 ]; then
    MAX=${AVR_OOO_ARRIVALS}
  fi
 
  if [ `echo "${AVR_OOO_ARRIVALS} < ${MIN}" | bc` == 1 ]; then
    MIN=${AVR_OOO_ARRIVALS}
  fi

  TOTAL=`echo "scale=5; ${TOTAL} + ${AVR_OOO_ARRIVALS}" | bc`
  CNT=$(( ${CNT} + 1 ))
done < ${FILE_NAME1}

TMP=`echo "scale=5; ${TOTAL} / ${CNT}" | bc` 
echo "AVR_OUT_OF_ORDER_ARRIVALS ${TMP}  MAX ${MAX} MIN ${MIN}" >> ${FILE_NAME1} 
echo "AVR_OUT_OF_ORDER_ARRIVALS ${TMP}  MAX ${MAX} MIN ${MIN}"

#rm ex${FILE_NAME}
#rm ex${FILE_NAME}-acc
