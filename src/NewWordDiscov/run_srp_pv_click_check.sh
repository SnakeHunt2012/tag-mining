DATE=$1
source ~/.bash_profile
shadoop fs -cat /home/hdp-guanggao/hive/warehouse/hdp_guanggao.db/nw_word_pair_join_entropy/ds=$DATE/* > word_pair_info.txt   
./word_pair_click_info_run.py  /home/hdp-guanggao/linjianguo/base_data/srp_pv_click_segment /home/hdp-guanggao/linjianguo/newworddiscov/word_pair_click_info/ds=$DATE


