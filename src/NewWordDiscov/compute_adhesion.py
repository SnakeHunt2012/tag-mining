import sys
import string

tcount  = 100000000
def  wilsonInterval(c, n):
    p = c/n;
    p = p + 1.96 * (1.96/(2*n) - (p*(1-p)/n + 1.96 * 1.96/(4*n*n)) ** 0.5)
    return p / (1 + 1.96 * 1.96/(2*n))

for line in sys.stdin:
    fields = line.strip().split("\t")
    if len(fields) != 5:
        continue;
    word = fields[0]
    co_word = fields[1]
    co_count = string.atof(fields[2])
    l_count =  string.atof(fields[3])
    r_count =  string.atof(fields[4])
    if wilsonInterval(co_count, l_count) < 0.01 and wilsonInterval(co_count, r_count) < 0.01:
        continue;
    if l_count < 1000 and  wilsonInterval(co_count, l_count) < 0.02:
        continue
    if  r_count < 1000 and wilsonInterval(co_count, r_count) < 0.02:
        continue;
    print word + "\t" + co_word +  "\t" + str(tcount * co_count / (l_count * r_count))






