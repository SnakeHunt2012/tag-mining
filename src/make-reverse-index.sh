#!/bin/bash

err_args=85

function usage ()
{
    echo "Usage: `basename $0` xxxx-xx-xx"
}

case "$1" in
    ""                    ) usage; exit ${err_args};;
    [0-9]*-[0-9]*-[0-9]*  ) DATE=$1;;
    *                     ) usage; exit ${err_args};;
esac

INPUT="/home/nlp/huangjingwen/data/category-title/ds=${DATE}"
OUTPUT="/home/nlp/huangjingwen/data/reverse-index/ds=${DATE}"

STREAMING=/usr/bin/hadoop/software/hadoop/contrib/streaming/hadoop-streaming.jar

alias shadoop='sudo -E -u nlp /usr/bin/hadoop/software/hadoop//bin/hadoop'
alias shive='sudo -u nlp -E /usr/bin/hadoop/software/hive//bin/hive'
shopt -s expand_aliases

shadoop fs -test -e $OUTPUT
if [ $? -eq 0 ]
then
    shadoop fs -rmr $OUTPUT
fi

shadoop jar $STREAMING \
-D mapred.linerecordreader.maxlength=1024000 \
-D mapred.job.priority=VERY_HIGH \
-D mapred.reduce.tasks=0 \
-D mapred.job.name='huangjingwen:make-reverse-index' \
-D map.output.key.field.separator='\t' \
-D stream.num.map.output.key.fields=1 \
-inputformat org.apache.hadoop.mapred.lib.CombineTextInputFormat \
-input $INPUT \
-output $OUTPUT \
-mapper './make-reverse-index tag-tree.bin' \
-file make-reverse-index \
-file tag-tree.bin \
-file qsegconf.ini \
-cacheArchive '/home/nlp/huangjingwen/lib/qmodule.tar.gz#qmodule'

