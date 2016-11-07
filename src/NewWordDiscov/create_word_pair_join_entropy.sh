source ~/.bash_profile
DATE=$1
expire=30
DEL_DATE=$(/bin/date -d "${DATE} $expire days ago" +%Y-%m-%d)

tablename=nw_word_pair_join_entropy
fromtable1=nw_word_pair_adhesion
fromtable2=nw_word_entropy

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
adhesion float,
lentropy float,
rentropy float
)
PARTITIONED BY (ds string)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '10'
STORED AS TEXTFILE;
alter table $tablename drop if exists partition(ds='$DATE');
insert overwrite  table $tablename partition(ds='$DATE') 
select t4.word, t4.co_word, t4.adhesion, t4.lentropy, t4.rentropy from  
(
select t12.word, t12.co_word, t12.adhesion, t12.lentropy, t3.rentropy
from
(
    select t1.word, t1.co_word, t1.adhesion, t2.lentropy 
    from
    (
        select * from $fromtable1 where(ds='$DATE' and adhesion > 1000)
    )t1
    left outer join
    (
        select word, entropy as lentropy from $fromtable2 where(ds='$DATE' and type = 'l' and entropy < 200 )
    )t2
    on
    (
        t1.co_word = t2.word
    )
)t12 
left outer join 
(   
    select word, entropy as rentropy from $fromtable2 where(ds='$DATE' and type = 'r' and entropy < 200)
)t3
on
(
    t12.word = t3.word
)
)t4 where(t4.lentropy is not NULL or t4.rentropy is not NULL);
alter table $tablename drop if exists partition(ds='$DEL_DATE') 

" | shive > ./hive_process.log
