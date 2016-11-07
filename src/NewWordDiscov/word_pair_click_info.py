#!/home/hdp-guanggao/muyixiang/python2.7/bin/python

import sys
from streaming.base import BaseMapper, BaseReducer
import string

#------------------------------------------------------------------------------
# modify these functions to implement your algorithm
#------------------------------------------------------------------------------

class Mapper(BaseMapper):
    def setup(self):
        self.dict_word_pair = {};
        f=file('word_pair_info.txt')
        for line in f:
            word_pair = line.strip().split("\t");
            if len(word_pair) >= 2:
                if word_pair[0] in self.dict_word_pair:
                    self.dict_word_pair[word_pair[0]].add(word_pair[1])
                else:
                    self.dict_word_pair[word_pair[0]] = set([word_pair[1]])

        self.special_char = set(['', '-', '/', '&'])

       #do something

    def map(self, key, value):
        fields = value.strip().split("\t");
        if len(fields) < 4:
            return;
        query_word_pair = set()
        title_word_pair = set()
        query_words = fields[0].split(" ");
        title_words = fields[2].split(" ");
        click = fields[3].strip();
        length = len(query_words)
        for i in xrange(0, length):
            if query_words[i] in self.dict_word_pair and i < length - 1:
                if query_words[i+1] in self.dict_word_pair[query_words[i]]:
                    query_word_pair.add(query_words[i] + " " + query_words[i+1]);
                elif i < length - 2 and query_words[i+1] in self.special_char:
                    tmpword = query_words[i+1] + query_words[i+2]
                    if tmpword in self.dict_word_pair:
                        query_word_pair.add(query_words[i] + " " + tmpword);
                        

        length = len(title_words)
        for i in xrange(0, length):
            if title_words[i] in self.dict_word_pair and i < length - 1:
                if title_words[i+1] in self.dict_word_pair[title_words[i]]:
                    title_word_pair.add(title_words[i] + " " + title_words[i+1]);
                elif i < length - 2 and title_words[i+1] in self.special_char:
                    tmpword = title_words[i+1] + title_words[i+2]
                    if tmpword in self.dict_word_pair:
                        title_word_pair.add(title_words[i] + " " + tmpword);
        for query_pair in query_word_pair:
            if query_pair in title_word_pair:
                yield query_pair, click + "\t" + click; 
            else:
                yield query_pair, click + "\t0";


        
        # print fields[7]
class Reducer(BaseReducer):
    def reduce(self, key, values):
        click_total = 0;
        match_total = 0;
        for value in values:
            click, match  = value.split("\t");
            try:
                click_total += string.atoi(click);
                match_total += string.atoi(match);
            except :
                pass

        yield string.replace(key, " ", "\t", 1),  str(click_total) + "\t" + str(match_total) 

mapper = Mapper()
combiner = Reducer()
reducer = Reducer()

###############################################################################
# framework (Do Not Modify)
###############################################################################

if __name__ == '__main__':
    from streaming.base import map_reduce_worker

    map_reduce_worker(sys.argv, mapper, reducer, combiner)


