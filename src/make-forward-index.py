#!/usr/bin/env python
# -*- coding: utf-8 -*-

from json import dumps
from argparse import ArgumentParser

def main():

    parser = ArgumentParser()
    parser.add_argument("reverse_index_file", help = "reverse index file (input)")
    parser.add_argument("forward_index_file", help = "reverse index file (output)")
    args = parser.parse_args()

    reverse_index_file = args.reverse_index_file
    forward_index_file = args.forward_index_file

    category_count_dict = {}
    tag_category_dict = {}
    with open(reverse_index_file, 'r') as fd:
        for line in fd:
            splited_line = line.strip().split("\t")
            if len(splited_line) != 2:
                continue
            category, tag_str = splited_line
            for tag in tag_str.split(" "):
                if tag not in tag_category_dict:
                    tag_category_dict[tag] = {}
                if category not in tag_category_dict[tag]:
                    tag_category_dict[tag][category] = 0
                tag_category_dict[tag][category] += 1
                if category not in category_count_dict:
                    category_count_dict[category] = 0
                category_count_dict[category] += 1
                
    with open(forward_index_file, 'w') as fd:
        for tag in tag_category_dict:
            fd.write("%s\t%s\n" % (tag, dumps(tag_category_dict[tag])))
            #category_score_list = [(score, category) for category, score in tag_category_dict[tag].iteritems()]
            #category_score_list.sort(reverse = True)
            #fd.write("%s\t%s\n" % (tag, " ".join(["%s:%d:%d:%f" % (category, score, category_count_dict[category], float(score) / category_count_dict[category]) for score, category in category_score_list])))
            #fd.write("%s\t%s\n" % (tag, " ".join(["%s:%d" % (category, score) for score, category in category_score_list if category not in set(["info", "news", "其他"])])))


if __name__ == "__main__":

    main()
