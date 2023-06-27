#! bin/bash
FILE_NAME1=$1
FILE_NAME2=$2
CON_NUM=$3
ACC_LEN=0

if [ $# -ne 3 ]; then
  echo "useage ./cat-con FILE NAME1 FILE_NAME2 CONNECTION_NUM"
  exit 0
fi

cut -d $' ' -f 1,3 result/${FILE_NAME1} > tmp1 
cut -d $' ' -f 1,3 result/${FILE_NAME2} > tmp2
cat tmp1 tmp2 > tmp3 
sort -n tmp3 > tmp4 

while read LINE
do
  TIME=`echo ${LINE} | sed -e 's/\([0-9]*.[0-9]*\).[0-9]*/\1/'`
  LEN=`echo ${LINE} | sed -e 's/[0-9]*.[0-9]*.\([0-9]*\)/\1/'`
  ACC_LEN=`expr ${ACC_LEN} + ${LEN}`
  echo ${TIME} ${ACC_LEN} >> result/con${CON_NUM}
done < tmp4 

for i in `seq 1 4` 
do
  rm tmp${i}
done

#rm ex${FILE_NAME}
#rm ex${FILE_NAME}-acc
