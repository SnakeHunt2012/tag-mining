#!/usr/bin/env python
# -*- coding: utf-8 -*-

from json import loads
from argparse import ArgumentParser

def main():

    parser = ArgumentParser()
    parser.add_argument("forward_index_file", help = "forward_index_file")
    args = parser.parse_args()

    forward_index_file = args.forward_index_file

    with open(forward_index_file, 'r') as fd:
        for line in fd:
            splited_line = line.strip().split("\t")
            if len(splited_line) != 2:
                continue
            tag, category_str = splited_line
            category_dict = loads(category_str)
            if u"info" in category_dict:
                category_dict.pop(u"info")
            if u"news" in category_dict:
                category_dict.pop(u"news")
            if u"其他" in category_dict:
                category_dict.pop(u"其他")
            count_sum = sum(category_dict.values())
            category_score_list = [(score, category) for category, score in category_dict.iteritems()]
            category_score_list.sort(reverse = True)
            if len(category_score_list) == 0:
                continue
            if category_score_list[0][0] <= 3:
                continue
            #if float(category_score_list[0][0]) / count_sum < 0.5:
            #    if len(category_score_list) >= 2 and category_score_list[1][0] >= 3 and float(category_score_list[0][0] + category_score_list[1][0]) / count_sum > 0.8:
            #        print tag, category_score_list[0][1].encode("utf-8"), category_score_list[1][1].encode("utf-8")
            #    continue
            #print tag, category_score_list[0][1].encode("utf-8")
            
            if float(category_score_list[0][0]) / count_sum < 0.8:
                if len(category_score_list) >= 2 and category_score_list[1][0] > 3 and float(category_score_list[0][0] + category_score_list[1][0]) / count_sum > 0.9:
                    print "%s\t%s" % (category_score_list[0][1].encode("utf-8"), tag)
                    print "%s\t%s" % (category_score_list[1][1].encode("utf-8"), tag)
                continue
            print "%s\t%s" % (category_score_list[0][1].encode("utf-8"), tag)

if __name__ == "__main__":

    main()
