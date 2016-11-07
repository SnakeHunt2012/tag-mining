#!/bin/bash

err_args=85
err_proc=86

alias shadoop='sudo -E -u nlp /usr/bin/hadoop/software/hadoop//bin/hadoop'
alias shive='sudo -u nlp -E /usr/bin/hadoop/software/hive//bin/hive'
shopt -s expand_aliases

query_pv_table="huangjingwen_query_pv"
query_pv_dir="/home/nlp/huangjingwen/data/query-pv"

function usage ()
{
    echo "Usage: `basename $0` xxxx-xx-xx"
}

case "$1" in
    ""                    ) usage; exit ${err_args};;
    [0-9]*-[0-9]*-[0-9]*  ) DATE=$1;;
    *                     ) usage; exit ${err_args};;
esac

bash create-query-pv-table.sh "${DATE}"

if [ $? -ne 0 ]
then
    echo "create-query-pv-table.sh failed"
    exit ${err_proc}
fi

shive -e "select query, pv from ${query_pv_table} where ds='${DATE}';" > query-pv.tsv
python segment.py query-pv.tsv seg-pv.tsv

shadoop fs -test -e "${query_pv_dir}/${DATE}"
if [ $? -eq 0 ]
then
    shadoop fs -rmr "${query_pv_dir}/${DATE}"
fi
shadoop fs -put seg-pv.tsv "${query_pv_dir}/${DATE}"

bash NewWordDiscov/run_nw_discov.sh "${DATE}"

if [ $? -eq 0 ]
then
    echo "SUCCESS output into table nw_word_pair_join_entropy"
    exit 0
fi
exit "${err_proc}"
