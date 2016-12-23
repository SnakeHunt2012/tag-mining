#!/usr/bin/env python
# -*- coding: utf-8 -*-

from math import log
from json import loads
from argparse import ArgumentParser

def entropy(category_count_dict):

    count_sum = sum(category_count_dict.values())
    category_proba_dict = dict((category, float(score) / count_sum) for category, score in category_count_dict.iteritems())
    return sum(score * log(score) for category, score in category_proba_dict.iteritems())

def select(category_count_dict, entropy_threshold, count_threshold):

    if len(category_count_dict) == 0:
        return None, None
    category_score_list = sorted([(score, category) for category, score in category_count_dict.iteritems()], reverse = True)
    score = entropy(category_count_dict)
    #if score > entropy_threshold and len(category_count_dict) > count_threshold:
    if score > entropy_threshold and category_count_dict[category_score_list[0][1]] > count_threshold:
        return category_score_list[0][1], score
    return None, None

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
            if len(category_dict) == 0:
                continue

            count_threshold = 3
            entropy_threshold = -1.7
            category, entropy_score = select(category_dict, entropy_threshold, count_threshold)
            if category:
                print "%s\t%s" % (category.encode("utf-8"), tag)
                #print "%s\t%s\t%f" % (category.encode("utf-8"), tag, entropy_score)
            else:
                continue

            category_dict.pop(category)
            #entropy_threshold /= 2
            count_threshold *= 10
            category, entropy_score = select(category_dict, entropy_threshold, count_threshold)
            if category:
                print "%s\t%s" % (category.encode("utf-8"), tag)
                #print "%s\t%s\t%f" % (category.encode("utf-8"), tag, entropy_score)
            else:
                continue
                
            category_dict.pop(category)
            #entropy_threshold /= 2
            count_threshold *= 10
            category, entropy_score = select(category_dict, entropy_threshold, count_threshold)
            if category:
                print "%s\t%s" % (category.encode("utf-8"), tag)
                #print "###debug###%s\t%s\t%f" % (category.encode("utf-8"), tag, entropy_score)
                #print "###debug###\t%r" % (category_dict)
            else:
                continue
                
            #category_dict.pop(category)
            #entropy_threshold /= 2
            #category = select(category_dict, entropy_threshold)
            #if category:
            #    print "%s\t%s" % (category.encode("utf-8"), tag)
                
            #if entropy(category_dict) > -1.7:
            #    print "%s\t%s" % (category_score_list[0][1].encode("utf-8"), tag)
            #    if len(category_score_list) >= 2 and category_score_list[1][0] > 3 and float(category_score_list[0][0] + category_score_list[1][0]) / count_sum > 0.9:
            #        print "%s\t%s" % (category_score_list[1][1].encode("utf-8"), tag)
            #        if len(category_score_list) >= 3 and category_score_list[2][0] > 3 and float(category_score_list[0][0] + category_score_list[1][0] + category_score_list[2][0]) / count_sum > 0.9:
            #            print "%s\t%s" % (category_score_list[2][1].encode("utf-8"), tag)

if __name__ == "__main__":

    main()
