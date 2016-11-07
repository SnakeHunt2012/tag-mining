DATE=$(/bin/date -d "1 days ago" +%Y-%m-%d)
if [ $# -gt 0 ]
then
    DATE=$1
fi
sh -x  NewWordDiscov.sh  $DATE 1 0 2  
sh -x create_word_co_count.sh $DATE
sh -x create_word_entropy.sh $DATE
sh -x create_word_pair_adhesion.sh $DATE
sh -x create_word_pair_join_entropy.sh $DATE
#sh -x run_srp_pv_click_check.sh $DATE
