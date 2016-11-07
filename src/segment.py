#!/usr/bin/env python
# -*- coding: utf-8 -*-

from jieba import cut
from argparse import ArgumentParser

def main():

    parser = ArgumentParser()
    parser.add_argument("source_file", help = "source file")
    parser.add_argument("target_file", help = "target file")
    args = parser.parse_args()

    source_file = args.source_file
    target_file = args.target_file

    with open(source_file, 'r') as source_fd:
        with open(target_file, 'w') as target_fd:
            for line in source_fd:
                splited_line = line.strip().split("\t")
                if len(splited_line) != 2:
                    continue
                query, pv = splited_line
                seg_list = [seg.encode("utf-8") for seg in cut(query)]
                target_fd.write("%s\t%s\n" % (" ".join(seg_list), pv))


if __name__ == "__main__":

    main()

