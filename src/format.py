#!/usr/bin/env python
# -*- coding: utf-8 -*-

from argparse import ArgumentParser

def main():

    parser = ArgumentParser()
    parser.add_argument("tag_file", help = "tag file (input)")
    parser.add_argument("format_file", help = "format file (output)")
    args = parser.parse_args()

    tag_file = args.tag_file
    format_file = args.format_file

    tag_set = set([])
    with open(tag_file, 'r') as fd:
        for line in fd:
            splited_line = line.strip().split("\t")
            if len(splited_line) != 1:
                continue
            tag = splited_line[0]
            if len(tag.decode("utf-8")) < 2:
                #print tag
                continue
            tag_set.add(tag)

    #print len(tag_set)

    with open(format_file, 'w') as fd:
        for index, item in enumerate(tag_set):
            fd.write("%s:%d\n" % (item, index))
    
        
if __name__ == "__main__":

    main()
