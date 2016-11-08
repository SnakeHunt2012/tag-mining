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

function assert_status ()
{
    if [ $? -ne 0 ]
    then
        echo "FAILED: $1"
        exit ${err_proc}
    fi
}

function assert_condition ()
{
    E_PARAM_ERR=98
    E_ASSERT_FAILED=99


    if [ -z "$2" ]          #  Not enough parameters passed
    then                    #+ to assert() function.
        return $E_PARAM_ERR #  No damage done.
    fi

    lineno=$2

    if [ ! $1 ]
    then
        echo "Assertion failed:  \"$1\""
        echo "File \"$0\", line $lineno"    # Give name of file and line number.
        exit $E_ASSERT_FAILED
        # else
        #   return
        #   and continue executing the script.
    fi  
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

#shive -e "select query, pv from ${query_pv_table} where ds='${DATE}';" > query-pv.tsv
#python segment.py query-pv.tsv seg-pv.tsv
#
#shadoop fs -test -e "${query_pv_dir}/ds=${DATE}"
#if [ $? -eq 0 ]
#then
#    shadoop fs -rmr "${query_pv_dir}/ds=${DATE}"
#fi
#shadoop fs -mkdir "${query_pv_dir}/ds=${DATE}"
#shadoop fs -put seg-pv.tsv "${query_pv_dir}/${DATE}"
#
#cd NewWordDiscov && bash run_nw_discov.sh "${DATE}" && cd ../
#assert_status "Write into table nw_word_pair_join_entropy."
#
#shive -e "select word, co_word from nw_word_pair_join_entropy where ds='${DATE}' and (adhesion > 5000 and lentropy > 0.9) or (adhesion > 300000); "  | awk -F '\t' '{print $1 $2}' > double-query.tsv
#assert_status "Dump double-query.tsv from table nw_word_pair_join_entropy"
#
#awk '{if (NF == 2) {print $1}}' seg-pv.tsv > single-query.tsv
#assert_status "Extract single queyr from seg-pv.tsv."
#
#: > tag.tsv
#cat single-query.tsv >> tag.tsv
#cat double-query.tsv >> tag.tsv
#
#sort tag.tsv | uniq > tag-uniq.tsv
#mv tag-uniq.tsv tag.tsv
#
#awk '{print $0 ":" NR}' tag.tsv > tag-dict.tsv
#python fasttrie.py -f 'T(c*):(l)\n' < tag-dict.tsv > tag-tree.bin
#assert_status "Build tag-tree.bin from tag-dict.tsv"
#
shive -e "select split(btag, '\\\\|')[0], title from news_url_content_tag;" > category-title.tsv
assert_status "Dump category-title.tsv from table news_url_content_tag."

make make-reverse-index
assert_status "Build make-reverse-index."
condition="-e make-reverse-index"
assert_condition "${condition}" ${LINENO}

./make-reverse-index tag-tree.bin category-title.tsv reverse-index.tsv
assert_status "Make reverse-index.tsv."
condition="-e reverse-index.tsv"
assert_condition "${condition}" ${LINENO}

python make-forward-index.py reverse-index.tsv forward-index.tsv
assert_status "Make forward-index.tsv."
condition="-e forward-index.tsv"
assert_condition "${condition}" ${LINENO}

python select-tag.py forward-index.tsv > tag-category.tsv
assert_status "Select tags from forward-index.tsv."
condition="-e tag-category.tsv"
assert_condition "${condition}" ${LINENO}
