#!/usr/bin/env python
# -*- coding: utf-8 -*-

from argparse import ArgumentParser

def main():

    parser = ArgumentParser()
    parser.add_argument("file_list", nargs = '+', help = "category-tag file list")
    args = parser.parse_args()

    file_list = args.file_list

    tag_category_dict = {}
    for file in file_list:
        with open(file, 'r') as fd:
            for line in fd:
                splited_line = line.strip().split("\t")
                if len(splited_line) != 2:
                    continue
                category, tag = splited_line
                if tag not in tag_category_dict:
                    tag_category_dict[tag] = set([])
                tag_category_dict[tag].add(category)

    for tag in tag_category_dict:
        for category in tag_category_dict[tag]:
            print "%s\t%s" % (category, tag)
                

if __name__ == "__main__":

    main()
