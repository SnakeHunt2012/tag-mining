DATE=$1
source ~/.bash_profile
DEL_DATE=$(/bin/date -d "${DATE} $expire days ago" +%Y-%m-%d)

tablename=nw_word_pair_adhesion
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
co_word string,
adhesion string
)
PARTITIONED BY (ds string)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '10'
STORED AS TEXTFILE;
add file compute_adhesion.py;
alter table $tablename drop if exists partition(ds='$DATE');
set mapred.reduce.tasks=15;
insert overwrite  table $tablename partition(ds='$DATE') 
select transform(t12.word, t12.co_word, t12.co_count, t12.lcount, t3.rcount) using 'python compute_adhesion.py' as word, co_word, adhesion 
from 
(
    select t1.word as word, t1.count as lcount, t2.co_word as co_word, t2.count as co_count 
    from
    (   
        select word, count from $fromtable where(ds='$DATE' and type = 's')
    )t1
    join
    (
        select word, co_word, count from $fromtable where(ds = '$DATE' and type = 'r')
    )t2
    on(t1.word = t2.word)
)t12 
join
(
    select word, count as rcount from $fromtable where(ds='$DATE' and type = 's')
)t3 
on
(t12.co_word = t3.word);


" | shive > ./hive_process.log
