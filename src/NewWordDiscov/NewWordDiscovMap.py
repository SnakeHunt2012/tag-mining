# coding=utf-8
import sys
import re
import string
from StringTool import *

segfields = set(int(x) for x in sys.argv[1].split(','))
fieldnum = int(sys.argv[2])
sepecial_char = set(['', '-', '/', '&'])

strTool = StringTool();
strTool.loadStopWord('stopword');

for line in sys.stdin:
    fields = line.rstrip().split("\t");
    if len(fields) < fieldnum :
        continue;
    if string.atoi(fields[fieldnum - 1].strip()) < 2:
        continue;
    for j in range(0, len(fields)):
        if j in segfields and fields[j] != '' and fields[j] != None: 
            strtmp  = re.sub("   *", "  ", fields[j]);
            words = strtmp.split(" ");
            length =  len(words)
            i = 0;
            while (i < length):
                if strTool.filterWord(words[i], 0x04) or words[i] == "":
                    i += 1;
                    continue;
                print words[i] + "\t1";
                if strTool.isSingleChinese(words[i]):
                    k = 1;
                    wordtmp = words[i];
                    while (i + k < length):
                        if  strTool.isSingleChinese(words[i+k]):
                            wordtmp += words[i+k];
                        else:
                            break;
                        k += 1
                    if k > 1:
                        print wordtmp +  "\t1";
                        if i + k < length:
                            print wordtmp + "\t" + words[i+k] + "\r1";

                if i + 1 < length:
                    if words[i+1] in sepecial_char and ord(words[i][0]) < 128:
                        if i + 2 < length and not strTool.filterWord(words[i+2], 0x04) and len(words[i+2]) > 0 and len(words[i+2]) < 10 and ord(words[i+2][0]) < 128 :
                            wordtmp = words[i+1] + words[i+2]
                            print words[i] + "\t" + wordtmp  + "\tr\t1"
                            print wordtmp + "\t1"
                            i += 3
                            continue
                        else:
                            i += 2
                            continue

                    if strTool.isSingleChinese(words[i+1]) : 
                        print words[i] + "\t" + words[i+1] + "\tr\t1";
                        wordtmp =  words[i+1];
                        k = 2;
                        while (i + k < length):
                            if (strTool.isSingleChinese(words[i+k])):
                                wordtmp += words[i+k]
                            else:
                                break;
                            k += 1;
                        print words[i] + '\t' + wordtmp + "\tr\t1";

                    elif strTool.filterWord(words[i+1], 0x04):
                        i += 2;
                        continue;
                    else:
                        print words[i] + "\t" + words[i+1] + "\tr\t1";
                i += 1;

