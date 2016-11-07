# coding=utf-8
import sys
import string
from urllib import unquote 

key = ""
sidset = set()
for line in sys.stdin:
    fields = line.strip().split("\t");
    if len(fields) < 3:
        continue;
    q, sid, count = fields;
    if key != q:
        if len(sidset) > 0 and key != "":
            print key + "\t" + str(len(sidset));
        key = q;
        sidset.clear()
    sidset.add(sid);

print key + "\t" + str(len(sidset));
