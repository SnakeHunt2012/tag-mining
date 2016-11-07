#!/bin/bash

if [ $# != 1 ]
then
	echo "args errors";
	exit
fi

period=45

DATE=$(/bin/date -d "$1" +%Y%m%d)
DEL_DATE=$(/bin/date -d "$period days ago $1" +%Y-%m-%d)

source ~/.bash_profile

INPUT=hdfs://w-namenode.qss.zzbc2.qihoo.net:9000/home/qss/data/sousuolog/${DATE}*/sou_srp*

OUTPUT=/home/hdp-guanggao/linjianguo/base_data/srp_pv/ds=$1


STREAMING=/usr/bin/hadoop/software/hadoop/contrib/streaming/hadoop-streaming.jar
shadoop fs -test -e $OUTPUT
if [ $? = 0 ]
then
    shadoop fs -rmr $OUTPUT
fi
shadoop jar $STREAMING \
-D mapred.linerecordreader.maxlength=1024000 \
-D mapred.reduce.tasks=300 \
-D mapred.job.priority=HIGH \
-D mapred.job.name='linjianguo:parse_srp_pv_log' \
-D map.output.key.field.separator='\t' \
-D stream.num.map.output.key.fields=1 \
-inputformat org.apache.hadoop.mapred.lib.CombineTextInputFormat \
-input $INPUT \
-output $OUTPUT \
-mapper 'python parse_srp_pv_map.py' \
-reducer 'python parse_srp_pv_reduce.py' \
-file parse_srp_pv_map.py \
-file parse_srp_pv_reduce.py


shadoop fs -test -e /home/hdp-guanggao/linjianguo/base_data/srp_pv/ds=$DEL_DATE
if [ $? = 0 ]
then
    shadoop fs -rmr /home/hdp-guanggao/linjianguo/base_data/srp_pv/ds=$DEL_DAT
fi



#-D mapred.reduce.tasks=500 \
