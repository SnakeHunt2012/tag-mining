DATE=$1
DEL_DATE=$(/bin/date -d "${DATE} $expire days ago" +%Y-%m-%d)
SDATE=$(/bin/date -d "${DATE}" +%Y%m%d)

tablename="nw_query_click_title_seg"
#if [ $# -eq 2 ] 
#then
#    if [ $2 -eq 1 ] 
#    then
#        echo " drop table ${tablename}; "  | $shive > ./log/hive_process.log
#    fi  
#fi


echo "
create external table if not exists $tablename (
query string,
query_seg string,
click int,
url string,
title_seg string
)
PARTITIONED BY (ds string)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '01'
LINES TERMINATED BY '10'
STORED AS TEXTFILE;
alter table $tablename drop if exists partition(ds='$DATE');
alter table $tablename add partition(ds='$DATE') location '/home/nlp/sijianfeng/cdrm/so.query2urltitle.seg/$SDATE'; 

" | sudo -u "nlp" -E $HIVE_HOME/bin/hive > ./hive_process.log
