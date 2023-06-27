#!/bin/bash
number = ${1}
for i in ${number}; do
	sed -e "s/192.168.10.4/192.168.20.4/" ./servers${number}.h > tmp
	mv tmp servers${number}.h
done 
	
