# coding=utf-8
import sys
import string
from urllib import unquote 

fieldnum = 11 
need_num = 4
for line in sys.stdin:
    fields = line.strip().split(" ");
    sid = "";
    query = "";
    if len(fields) < fieldnum:
        continue;
   # print fields[7]
    clickinfo = fields[7].split('&');
   
   #print len(clickinfo)
    para_num = 0;
    for para in clickinfo:
        if para_num == 2:
            break;
        para_kv = para.split('=');
        if para_kv[0] == 'sid':
            sid = para_kv[1];
            para_num += 1;
        elif para_kv[0] == 'q':
            query = para_kv[1];
            para_num += 1;

   # print sid + "\t" + query + "\t" + url + "\t" + psid
    if query != "" and sid != "" :
        query = unquote(query)
        print query + "\t" + sid + "\t1
