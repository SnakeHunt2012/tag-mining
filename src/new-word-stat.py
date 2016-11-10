#!/usr/bin/env python
# -*- coding: utf-8 -*-

from argparse import ArgumentParser
from matplotlib import pyplot as plt

def main():

    parser = ArgumentParser()
    parser.add_argument("new_word_file", help = "new word file")
    args = parser.parse_args()

    new_word_file = args.new_word_file

    adhesion_list = []
    lentropy_list = []
    rentropy_list = []
    with open(new_word_file, 'r') as fd:
        for line in fd:
            splited_line = line.strip().split("\t")
            if len(splited_line) != 6:
                continue
            word, co_word, adhesion, lentropy, rentropy, _ = splited_line
            adhesion_list.append(float(adhesion))
            lentropy_list.append(float(lentropy))
            rentropy_list.append(float(rentropy))

    plt.boxplot(adhesion_list)

if __name__ == "__main__":

    main()
