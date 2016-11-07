#!/bin/bash

if [ $# != 4 ]
then
	echo "args errors";
	exit
fi
source ~/.bash_profile
DATE=$1
DAYS=$2

INPUT=/home/nlp/huangjingwen/data/query-pv/ds=$DATE
for ((i=1; i<$DAYS; i++))
do
    theday=$(/bin/date -d " $i days ago $DATE " +%Y-%m-%d)

    shadoop fs -test -e /home/nlp/huangjingwen/data/query-pv/ds=$theday
    if [ $? = 0 ]
    then
        INPUT=$INPUT,/home/nlp/huangjingwen/data/query-pv/ds=$theday
    fi
done


OUTPUT=/home/nlp/huangjingwen/data/word-count/ds=$DATE
FIELDS=$3
FIELDNUM=$4

STREAMING=/usr/bin/hadoop/software/hadoop/contrib/streaming/hadoop-streaming.jar
shadoop fs -test -e $OUTPUT
if [ $? = 0 ]
then
    shadoop fs -rmr $OUTPUT
fi
shadoop jar $STREAMING \
-D mapred.linerecordreader.maxlength=1024000 \
-D mapred.reduce.tasks=500 \
-D mapred.job.priority=VERY_HIGH \
-D mapred.job.name='huangjingwen:newworddiscov' \
-D map.output.key.field.separator='\t' \
-D stream.num.map.output.key.fields=1 \
-inputformat org.apache.hadoop.mapred.lib.CombineTextInputFormat \
-input $INPUT \
-output $OUTPUT \
-mapper 'python NewWordDiscovMap.py '$FIELDS' '$FIELDNUM \
-reducer 'python NewWordDiscovReduce.py ' \
-file NewWordDiscovMap.py \
-file NewWordDiscovReduce.py \
-file StringTool.py \
-file stopword

