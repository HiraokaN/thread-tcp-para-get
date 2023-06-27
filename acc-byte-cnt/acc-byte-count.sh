#! bin/bash
FILE_NAME=$1
#count=0
#cnt=0

if [ $# -ne 1 ]; then
  echo "useage ./acc-byte-cnt FILE NAME"
  exit 0
fi

bash extract.sh ${FILE_NAME} > ex${FILE_NAME}
./byte-cnt ex${FILE_NAME}
bash div-flow.sh ex${FILE_NAME}-acc

#rm ex${FILE_NAME}
#rm ex${FILE_NAME}-acc
