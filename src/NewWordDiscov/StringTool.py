#--*-- coding=utf-8 --*--

import sys
import string
import unicodedata

class StringTool:
    def __init__(self):
        self._stopWord = set()
        self.name = "StringTool"
        self._chinesePunctuation = set()
        chinesePunctuation = "。，、＇：∶；‘’“”〝〞ˆˇ﹕︰﹔﹖﹑¨….¸;！´？！～—ˉ｜‖＂〃｀@﹫¡¿﹏﹋﹌︴々﹩﹠&﹪%*﹡﹢﹦﹤‐￣¯―﹨ˆ˜﹍﹎+=<­­＿_-\ˇ~﹉﹊（）〈〉‹›﹛﹜『』〖〗［］《》〔〕{}「」【】︵︷︿︹︽_﹁﹃︻︶︸﹀︺︾ˉ﹂﹄︼★◢"
        for i in range(0, len(chinesePunctuation)-3,3):
            word = chinesePunctuation[i:i+3];
            self._chinesePunctuation.add(word);

    def loadStopWord(self, stopWordFile):
        f = file(stopWordFile, 'r');
        for line in f:
            self._stopWord.add(line.strip());

    def filterStopWord(self, word):
        if word in self._stopWord:
            return 1;
        return 0;

    def filterPunctuation(self, word):
        #filter ascii punctuation
        if len(word) == 1 :
            ascii =  ord(word)
            print ascii;
            if ascii <= 47:
                return 1;
            if ascii >= 58 and ascii <= 64:
                return 1;
            if ascii >= 91  and ascii <= 96:
                return 1;
            if ascii >= 123:
                return 1;
        #filter chinese punctuation:
        #if len(word) == 3 and word in self._chinesePunctuation:
        #    return 1;
        if len(word) == 3:
            code = (ord(word[0]) << 8) + ord(word[1])
            if code < 0xE4B8  or code > 0xE9BE:
                return 1;
        return 0;
    
    def isSingleChar(self, word):
            if len(word) <= 1:
                return 1;
            if len(word) == 2 and ord(word[0]) > 0XC0:
                return 1;
            if len(word) == 3 and ord(word[0]) > 0xE0:
                return 1;
            if len(word) == 4 and ord(word[0]) > 0xF0:
                return 1;
            return 0;
     
    def isSingleChinese(self, word):
        if len(word) == 3:
            code0 = ord(word[0])
            code1 = ord(word[1])
            code = (ord(word[0]) << 8 ) + ord(word[1])
            if code0 >= 0xE4B8 and code0 <= 0xE9BE:
                return 1;
            return 0

    def isDigit(self, word):
        if (word[0] < '0' or word[0] > '9') and word[0] != '-':
            return 0;
        try:
            float(word)
            return 1;
        except (TypeError, ValueError):
            pass
        return 0;
    
    #**def option 1 : filter single char **
    #**           2 : filter stop word   **
    #**           4 : filter punctuation **
    #**           8 : filter digit       **
    def filterWord(self, word, option):
        if option & 0x01:
            if self.isSingleChar(word):
                return 1;
        if option & 0x02:
            if self.filterStopWord(word):
                return 1;
        if option & 0x04:
            if len(word) == 1 or len(word) ==3 :
                if self.filterPunctuation(word):
                    return 1;
        if option & 0x08:
            if self.isDigit(word):
                return 1;
        return 0;
if __name__ == '__main__':  
    strTool = StringTool();
    if strTool.filterWord("", 0x04):
        print "yes"
    

   # chnpunc = "《》（）%￥#@！{}【】？"

   # print str;

