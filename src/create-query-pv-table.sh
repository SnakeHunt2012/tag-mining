#!/bin/bash

GROUP="nlp"
query_pv_table="huangjingwen_query_pv"
url_table="nw_query_click_title_seg"

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

SDATE=$(/bin/date -d "${DATE} 30 days ago" +%Y-%m-%d)

echo "
create external table if not exists ${query_pv_table} (
query string,
pv int
) 
PARTITIONED BY (ds string) 
ROW FORMAT DELIMITED 
FIELDS TERMINATED BY '\t' 
LINES TERMINATED BY '\n' 
STORED AS TEXTFILE;

alter table ${query_pv_table} drop if exists partition(ds='${DATE}');
insert overwrite table ${query_pv_table} partition(ds='$DATE')
select t1.query, t1.pv
from (
    select query, count(*) as pv from nw_query_click_title_seg where ds > '${SDATE}' group by query
) t1
where t1.pv > 5;
" | sudo -u $GROUP -E $HIVE_HOME/bin/hive > ./hive.log

