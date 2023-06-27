#!/bin/bash
NUMBER="${1}"
for i in `seq 1 ${NUMBER}`; do
	sed -e "s/192.168.10.4/192.168.20.4/" ./servers${i}.h > tmp
	mv tmp servers${i}.h
done 
	
