DATE=$1
source ~/.bash_profile
DEL_DATE=$(/bin/date -d "${DATE} $expire days ago" +%Y-%m-%d)

tablename=nw_word_entropy
fromtable=nw_word_co_count

if [ $# -eq 2 ] 
then
    if [ $2 -eq 1 ] 
    then
        echo " drop table ${tablename}; "  | shive > ../log/hive_process.log
    fi  
fi


echo "

create external table if not exists $tablename(
word string,
entropy float,
type string
)
PARTITIONED BY (ds string)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '10'
STORED AS TEXTFILE;
alter table $tablename drop if exists partition(ds='$DATE');
insert overwrite  table $tablename partition(ds='$DATE') 
select t12.word, -1.0 * sum(t12.logp),  t12.type
from
(
    select t1.word, t1.type, (log(1.0 * t1.count / t2.total ) * t1.count / t2.total) as logp from
    (
        select word, 'r' as type,  count from $fromtable where(ds = '$DATE' and type =  'r')
        union all 
        select co_word as word,  'l' as type, count from $fromtable where (ds='$DATE' and  type = 'r')
    )t1
    join
    (
        select word, 'r' as type, sum(count) as total from $fromtable where(ds='$DATE' and  type = 'r') group by word
        union all 
        select co_word as word, 'l' as type, sum(count) as total from $fromtable where(ds='$DATE' and type = 'r') group by co_word
    )t2
    on
    (t1.word = t2.word and t1.type = t2.type)
)t12  group by t12.word, t12.type;


" | shive > ./hive_process.log
#set hive.input.format=org.apache.hadoop.hive.ql.io.CombineHiveInputFormat;
