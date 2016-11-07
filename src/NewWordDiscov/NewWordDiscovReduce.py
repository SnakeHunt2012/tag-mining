# coding=utf-8
import sys
import string

rword_count = {}
key = "";
freq = 0;
freqThres = 2
co_freqThres = 3
co_probThres = 0.0005
def PrintInfo(key, freq, rword_count):
    global freqThres
    global co_freqThres
    global co_probThres
    if freq < freqThres:
        return;
    print key + "\t\t" + str(freq) + "\ts"
    for word, count in rword_count.items():
        if (count >= co_freqThres) and (float(count) / freq > co_probThres):
            print key + "\t" + word + "\t" + str(count) + "\tr";

for line in sys.stdin:
    strlist = line.strip().split("\t");
    if len(strlist) == 2:
        word, count = strlist;
        if key != word:
            PrintInfo(key, freq, rword_count);
            key = word;
            freq = 0;
            rword_count = {}
        freq += string.atoi(count);
    elif len(strlist) == 4:
        word, nb_word, pos, count = strlist
        if key != word:
            PrintInfo(key, freq, rword_count);
            key = word;
            freq = 0;
            rword_count = {}
        if pos == 'r':
            if nb_word in rword_count:
                rword_count[nb_word] += string.atoi(count)
            else:
                rword_count[nb_word] = string.atoi(count)
        else:
            continue;
    else:
        continue

PrintInfo(key, freq, rword_count)
rword_count = {}

        
