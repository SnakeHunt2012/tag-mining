#!/bin/sh

if [ $# -ne 3 ]
then
	echo "please offer args"
	exit
fi

#if [ $1 == "map" ]
#then
#	/usr/bin/python2.6 seg.py map $2
#chmod a+x ./cpp_seg_tool 
tar -zxf seg.tar.gz;
export LD_LIBRARY_PATH=./lib/:$LD_LIBRARY_PATH
./cpp_seg_tool $1 $2 $3 '	'

