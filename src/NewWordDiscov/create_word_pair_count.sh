DATE=$1
source ~/.bash_profile
DEL_DATE=$(/bin/date -d "${DATE} $expire days ago" +%Y-%m-%d)


if [ $# -eq 2 ] 
then
    if [ $2 -eq 1 ] 
    then
        echo " drop table ${tablename}; "  | shive > ../log/hive_process.log
    fi  
fi

tablename=nw_word_pair_join_lcount
fromtable=nw_word_co_count
echo "
create external table if not exists $tablename(
word string,
co_word string,
co_count int,
lcount int
)
PARTITIONED BY (ds string)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '10'
STORED AS TEXTFILE;
alter table $tablename drop if exists partition(ds='$DATE');
set mapred.resuce.tasks=15;
insert overwrite  table $tablename partition(ds='$DATE') 
    select t1.word as word, t2.co_word as co_word, t2.count as co_count, t1.count as lcount
    from
    (   
        select word, count from $fromtable where(ds='$DATE' and type = 's')
    )t1
    join
    (
        select word, co_word, count from $fromtable where(ds = '$DATE' and type = 'r')
    )t2
    on(t1.word = t2.word);

" | shive > ../log/hive_process.log

tablename=nw_word_pair_count
fromtable=nw_word_co_count
echo "
create external table if not exists $tablename(
word string,
co_word string,
co_count int,
lcount int,
rcount int
)
PARTITIONED BY (ds string)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '10'
STORED AS TEXTFILE;
alter table $tablename drop if exists partition(ds='$DATE');
set mapred.resuce.tasks=15;
insert overwrite  table $tablename partition(ds='$DATE') 
select t12.word, t12.co_word, t12.co_count, t12.lcount, t3.rcount
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


" | shive > ../log/hive_process.log
