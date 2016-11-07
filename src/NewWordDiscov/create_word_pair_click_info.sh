DATE=$1
source ~/.bash_profile
DEL_DATE=$(/bin/date -d "${DATE} $expire days ago" +%Y-%m-%d)

tablename=nw_word_pair_click_info
if [ $# -eq 2 ] 
then
    if [ $2 -eq 1 ] 
    then
        echo " drop table ${tablename}; "  | shive > ../log/hive_process.log
    fi  
fi


echo "
create external table if not exists $tablename(
word  string,
co_word string,
click bigint,
match bigint
)
PARTITIONED BY (ds string)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '10'
STORED AS TEXTFILE;
alter table $tablename drop if exists partition(ds='$DATE');
alter table $tablename add partition(ds='$DATE') location '/home/hdp-guanggao/linjianguo/newworddiscov/word_pair_click_info/ds=$DATE'; 

" | shive > ../log/hive_process.log
