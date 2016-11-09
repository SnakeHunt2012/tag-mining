#!/usr/bin/env python

# FastTrie 2.4.10 2014-08-11

# NOTE: This python program mixes with C++ code. It's recommended adding
#       following lines (without the #) in your .vimrc for better reading:
#
# au BufReadPost *.py unlet b:current_syntax
# au BufReadPost *.py syntax include @Cpp syntax/cpp.vim
# au BufReadPost *.py syntax region CppRegion keepend contains=@Cpp
#     \ start=+"" """+ end=+""" ""+
# au BufReadPost *.py syntax sync fromstart

import sys, optparse, re, os, tempfile, shutil, signal

if   sys.version_info >= (2, 5):
	import subprocess, hashlib
elif sys.version_info >= (2, 4):
	import subprocess, md5
else:
	import popen2, md5

# parse command line

def print_help(option, opt, value, parser):
	parser.print_help()

	if sys.stdout.isatty():
		F = lambda text: "\x1B[35m"    + text + "\x1B[0m"
		T = lambda text: "\x1B[33m"    + text + "\x1B[0m"
		S = lambda text: "\x1B[36m"    + text + "\x1B[0m"
		A = lambda text: "\x1B[31m"    + text + "\x1B[0m"
		O = lambda text: "\x1B[01;30m" + text + "\x1B[0m"
	else:
		O = lambda text: text; F = O; T = O; S = O; A = O

	sys.stdout.write("\nSupported "+F("Format")+"s:\n"\
	"  Struct                                    "+T("Type")+S("Sep")+O("[")+T("Type")+S("Sep")+O("...]")+"\n"\
	"  Vector<Struct>                            "+T("Type")+S("Sep")+O("[")+T("Type")+S("Sep")+O("...]")+"*\n"\
	"  Vector<ValueT, "+A("SizeT")+">                     V"+O("[")+","+A("Arg")+O("...]")+"("+F("Value")+")"+S("Sep")+"\n"\
	"  Trie<KeyT, ValueT, "+A("option")+", "+A("CharT")+", "+A("SizeT")+">  T"+O("[")+","+A("Arg")+O("...]")+"("+F("Key")+")"+S("Sep")+"("+F("Value")+")"+S("Sep")+"\n"\
	"  HashMap<KeyT, ValueT, "+A("HashT")+", "+A("SizeT")+">       H"+O("[")+","+A("Arg")+O("...]")+"("+F("Key")+")"+S("Sep")+"("+F("Value")+")"+S("Sep")+"\n"\
	"  Pair<ValueT1, ValueT2>                    P("+F("Value")+")"+S("Sep")+"("+F("Value")+")\n")
	sys.stdout.write("\nSupported "+T("Type")+"s:\n"\
	"  "+T("c")+" char     "+T("C")+" unsigned char         "+T("u")+" utf-8/16 char\n"\
	"  "+T("b")+"  int8_t  "+T("s")+"  int16_t  "+T("l")+"  int32_t  "+T("q")+"  int64_t  "+T("f")+" float\n"\
	"  "+T("B")+" uint8_t  "+T("S")+" uint16_t  "+T("L")+" uint32_t  "+T("Q")+" uint64_t  "+T("d")+" double\n")
	sys.stdout.write("\nSupported "+S("Seperator")+"s:\n"\
	"  any character except 0-9 A-Z a-z * ( and )\n"\
	"  supports "+S("\\t")+" "+S("\\n")+", etc. "+S("\\x1B")+" etc. and multi-characters\n")
	sys.stdout.write("\nSupported "+A("Argument")+"s:\n"\
	"  option  "+A("FT_NOTAIL")+" | "+A("FT_PATH")+" | "+A("FT_QUICKBUILD")+"\n"\
	"  CharT   "+A("uint8_t")+" or "+A("uint16_t")+"\n"\
	"  SizeT   "+A("uint8_t")+" or "+A("uint16_t")+" or "+A("uint32_t")+" or "+A("uint64_t")+"\n"\
	"  HashT   any function type\n")
	sys.stdout.write("\nExamples:\n"\
	"  Vector<pair<uint32_t, float> >           V("+T("L")+S(":")+T("f")+")"+S(",")+"\n"\
	"  Vector<pair<uint32_t, float> >           "+T("L")+S(":")+T("f")+S(",")+"*\n"\
	"  Vector<pair<uint32_t, float>, uint64_t>  V,"+A("uint64_t")+"("+T("L")+S(":")+T("f")+")"+S(",")+"\n"\
	"  Vector<pair<uint32_t, float>, uint64_t>  "+T("L")+S(":")+T("f")+S(",")+"**\n"\
	"  Trie<String, Vector<String> >            T("+T("c")+"*)"+S("\\t")+"(V("+T("c")+"*)"+S("\\x01")+")"+S("\\n")+"\n"\
	"  Trie<String, double, FT_PATH>            T,"+A("FT_PATH")+"("+T("c")+"*)"+S("\\t")+"("+T("d")+")"+S("\\n")+"\n"\
	"  HashMap<uint32_t, Pair<String, float> >  H("+T("L")+")"+S("\\t")+"(P("+T("c")+"*)"+S(":")+"("+T("f")+"))"+S("\\n")+"\n"\
	"  Pair<String, HashMap<uint32_t, float> >  P("+T("c")+"*)"+S("\\n\\n")+"(H("+T("L")+")"+S("\\t")+"("+T("f")+")"+S("\\n")+")\n")

	sys.exit(-1)

parser = optparse.OptionParser(add_help_option = False,
		usage = "\n  %prog [options] < input.txt > output.ft"
						"\n  %prog [options] input.ft .. > output.txt")
parser.add_option("-f", "--format", default = r'T(c*)\n',
		help = "specify container format string (default: '%default')")
parser.add_option("-d", "--disk",  action = "store_true", default = False,
		help = "swap intermediate data on disk during building (default: in memory)")
parser.add_option("-p", "--print", action = "store_true", default = False,
		help = "print Trie values by manually inputing keys", dest = "printing")
parser.add_option("-i", "--index", metavar = "FORMAT", default = r'',
		help = "create index to container specified by format string", dest = "index")
parser.add_option("-r", "--inverse", metavar = "FORMAT", default = r'',
		help = "create inverse to container specified by format string", dest = "inverse")
parser.add_option("-I", "--include", metavar = "DIR",
		help = "specify the path to FastTrie.h and MMap.h")
parser.add_option("-x", "--extend", metavar = "FILE", action = "append", default = [],
		help = "include some C/C++ source file for use in HashMap")
parser.add_option("-c", "--compile", metavar = "DIR",
		help = "generate and compile C++ source code in DIR (default: $FASTTRIE_COMPILE)")
parser.add_option("-h", "--help", action = "callback", callback = print_help,
		help = "print help")

(options, args) = parser.parse_args()

# parse container format string

class Container:
	pttnType      = r'(?:[cCbBsSlLqQfdu])' # type (e.g. char, uint32_t)
	pttnSeparator = r'(?:\\x[0-9A-Fa-f]{2}|\\[0-3][0-7]{2}|\\.|[^*()\\])'          # type separator (e.g. '\na123:\n')
	pttnWeakerSep = r'(?:\\x[0-9A-Fa-f]{2}|\\[0-3][0-7]{2}|\\.|[^0-9A-Za-z*()\\])' # weaker type separator (e.g. '\t')
	pttnTarg      = r'(?:(?:,[^,()]+)+)'   # template arguments (e.g. FT_PATH)

	# match a struct or struct sequence
	pttnTypeSeq   = r'(?P<pre>' + pttnWeakerSep + r'*)' \
			+ r'(?P<seq>(?:' + pttnType + pttnWeakerSep + r'*)+)(?P<rep>\*{,2})'
	# match a Vector
	pttnVector    = r'V(?P<arg>(?:' + pttnTarg + r')?)' \
			+ r'\((?P<sub>.+)\)(?P<sep>' + pttnSeparator + r'+)'
	# match a Trie<bool>
	pttnTrieSet   = r'T(?P<arg>(?:' + pttnTarg + r')?)' \
			+ r'\((?P<key>' + pttnTypeSeq + r')\)(?P<keysep>' + pttnSeparator + r'+)'
	# match a Trie
	pttnTrie      = r'T(?P<arg>(?:' + pttnTarg + r')?)' \
			+ r'\((?P<key>' + pttnTypeSeq + r')\)(?P<keysep>' + pttnSeparator + r'+)' \
			+ r'\((?P<sub>.+)\)(?P<sep>' + pttnSeparator + r'+)'
	# match a HashMap<bool>
	pttnHashSet   = r'H(?P<arg>(?:' + pttnTarg + r')?)' \
			+ r'\((?P<key>' + pttnTypeSeq + r')\)(?P<keysep>' + pttnSeparator + r'+)'
	# match a HashMap
	pttnHashMap   = r'H(?P<arg>(?:' + pttnTarg + r')?)' \
			+ r'\((?P<key>' + pttnTypeSeq + r')\)(?P<keysep>' + pttnSeparator + r'+)' \
			+ r'\((?P<sub>.+)\)(?P<sep>' + pttnSeparator + r'+)'

	type2codes = {
			"c": "char",    "C": "unsigned char",
			"b": "int8_t",  "B": "uint8_t",
			"s": "int16_t", "S": "uint16_t",
			"l": "int32_t", "L": "uint32_t",
			"q": "int64_t", "Q": "uint64_t",
			"f": "float",   "d": "double",   "u": "uint16_t",
	}

	# generate containers recursively
	# self.type: C++ type of the container, e.g. Trie<Struct_1, Vector<Struct_2> >
	# self.diskType: ft2 type for saving intermediate data on disk
	# self.code: C++ code for reading or writing the C++ type above
	# self.size: How many sub containers
	def __init__(self, n, format):
		if re.match(self.pttnTypeSeq + '$', format):
			self.m = re.match(self.pttnTypeSeq + '$', format)

			if self.m.group("pre") + self.m.group("seq") == format:
				# a struct
				self.type     = "Struct_" + str(n) + " "
				self.diskType = "Struct_" + str(n) + " "
				self.code = self.format2Struct(n) \
						+ self.format2getStruct(n) + self.format2putStruct(n)
				self.size = 1
				self.isStructOrSequence = True
			else:
				# a struct sequence
				self.type     = "Vector<Struct_" + str(n) \
						+ (self.m.group("rep") == '*' and " " or ", uint64_t") + " > "
				self.diskType = "Vector<Struct_" + str(n) \
						+ (self.m.group("rep") == '*' and " " or ", uint64_t") + " > "
				self.code = self.format2Struct(n) \
						+ self.format2getVectorStruct(n) + self.format2putVectorStruct(n)
				self.size = 2
				self.isStructOrSequence = True
		elif re.match(self.pttnVector + '$', format):
			self.m = re.match(self.pttnVector + '$', format)

			self.sub = Container(n + 1, self.m.group("sub"))

			# a Vector
			self.type     = "Vector<" + self.sub.type     + self.m.group("arg") + " > "
			self.diskType = "Vector<" + self.sub.diskType + self.m.group("arg") + " > "
			self.code = self.sub.code \
					+ self.format2getVector(n) + self.format2putVector(n)
			self.size = self.sub.size + 1
		elif re.match(self.pttnTrieSet + '$', format):
			self.m = re.match(self.pttnTrieSet + '$', format)

			self.key = Container(n + 1, self.m.group("key"))

			# a Trie<bool>
			self.type     = "Trie<" + self.key.type \
					+ ", bool" + self.m.group("arg") + " > "
			self.diskType = "Vector<Pair<" + self.key.diskType + ", bool>, size_t> "
			self.code = self.key.code \
					+ self.format2getTrieSet(n) + self.format2putTrieSet(n)
			self.size = self.key.size + 2
		elif re.match(self.pttnTrie + '$', format):
			self.m = re.match(self.pttnTrie + '$', format)

			self.key = Container(n + 1,                 self.m.group("key"))
			self.sub = Container(n + 1 + self.key.size, self.m.group("sub"))

			# a Trie
			self.type     = "Trie<" + self.key.type \
					+ ", " + self.sub.type + self.m.group("arg") + " > "
			self.diskType = "Vector<Pair<" + self.key.diskType \
					+ ", " + self.sub.diskType + ">, size_t> "
			self.code = self.key.code + self.sub.code \
					+ self.format2getTrie(n) + self.format2putTrie(n)
			self.size = self.key.size + self.sub.size + 1
		elif re.match(self.pttnHashSet + '$', format):
			self.m = re.match(self.pttnHashSet + '$', format)

			self.key = Container(n + 1, self.m.group("key"))

			# a HashMap<bool>
			self.type     = "HashMap<" + self.key.type \
					+ ", bool" + self.m.group("arg") + " > "
			self.diskType = "Vector<Pair<" + self.key.diskType + ", bool>, size_t> "
			self.code = self.key.code \
					+ self.format2getTrieSet(n) + self.format2putTrieSet(n) # use Trie's
			self.size = self.key.size + 2
		elif re.match(self.pttnHashMap + '$', format):
			self.m = re.match(self.pttnHashMap + '$', format)

			self.key = Container(n + 1,                 self.m.group("key"))
			self.sub = Container(n + 1 + self.key.size, self.m.group("sub"))

			# a HashMap
			self.type     = "HashMap<" + self.key.type \
					+ ", " + self.sub.type + self.m.group("arg") + " > "
			self.diskType = "Vector<Pair<" + self.key.diskType \
					+ ", " + self.sub.diskType + ">, size_t> "
			self.code = self.key.code + self.sub.code \
					+ self.format2getTrie(n) + self.format2putTrie(n) # use Trie's
			self.size = self.key.size + self.sub.size + 1
		else:
			for m in re.finditer(r'(?P<sep>' + self.pttnSeparator + r'+)', format):
				try:
					if m.start() < 4 or m.end() > len(format) - 3: continue

					self.sub1 = Container(n,                  format[2:m.start() - 1])
					self.sub2 = Container(n + self.sub1.size, format[m.end() + 1:- 1])

					# match a Pair
					pttnPair = r'P' \
							+ r'\((?P<sub1>' + re.escape(self.sub1.m.group(0)) + r')\)' \
							+ r'(?P<sep>' + self.pttnSeparator + r'+)' \
							+ r'\((?P<sub2>' + re.escape(self.sub2.m.group(0)) + r')\)'

					self.m = re.match(pttnPair + '$', format)
				except:
					continue
				break
			else:
				raise ValueError("incorrect format string '" + format + "'")

			# a Pair
			self.type     = "Pair<" + self.sub1.type     + ", " + self.sub2.type     + "> "
			self.diskType = "Pair<" + self.sub1.diskType + ", " + self.sub2.diskType + "> "
			self.code = self.sub1.code + self.sub2.code \
					+ self.format2getPair(n) + self.format2putPair(n)
			self.size = self.sub1.size + self.sub2.size

	# generate struct
	def format2Struct(self, n):
		matches = [x for x in map(lambda x: x, re.finditer(
				'(' + self.pttnType + ')(' + self.pttnWeakerSep + '*)', self.m.group("seq")))]

		if len(matches) == 1:
			return "typedef " \
					+ self.type2codes[matches[0].group(1)] + " Struct_" + str(n) + ";\n\n"

		code = "struct Struct_" + str(n) + "\n{\n"

		if len(matches) == 2:
			code += "\ttypedef " + self.type2codes[matches[0].group(1)] + " first_type;\n"
			code += "\ttypedef " + self.type2codes[matches[1].group(1)] + " second_type;\n\n"
			code += "\ttemplate <class P>\n"
			code += "\tStruct_" + str(n) + "(P x) : first(x.first), second(x.second) {}\n\n"

		for v in range(len(matches)):
			type = matches[v].group(1)
			name = len(matches) == 2 and v == 0 and "first" \
					or len(matches) == 2 and v == 1 and "second" or "v" + str(v)

			code += "\t" + self.type2codes[type] + " " + name + ";\n"

		# constructor and copy constructor to fill struct with 0
		code += "\n\tStruct_" + str(n) + "()\n\t{\n"
		code += "\t\tmemset(this, 0, sizeof(Struct_" + str(n) + "));\n"
		code += "\t}\n"

		code += "\n\tStruct_" + str(n) + "(const Struct_" + str(n) + " &x)\n\t{\n"
		code += "\t\tmemset(this, 0, sizeof(Struct_" + str(n) + "));\n\n"
		for v in range(len(matches)):
			name = len(matches) == 2 and v == 0 and "first" \
					or len(matches) == 2 and v == 1 and "second" or "v" + str(v)
			code += "\t\t" + name + " = x." + name + ";\n"
		code += "\t}\n"

		# need operator < to use Struct as key in std::map
		code += "\n\tbool operator < (const Struct_" + str(n) + " &x) const\n\t{\n"
		for v in range(len(matches)):
			name = len(matches) == 2 and v == 0 and "first" \
					or len(matches) == 2 and v == 1 and "second" or "v" + str(v)
			code += "\t\tif (" + name + " < x." + name + ") return true;\n"
			code += "\t\tif (" + name + " > x." + name + ") return false;\n"
		code += "\n\t\treturn false;\n\t}\n"

		# need operator == to use Struct as key in HashMap
		code += "\n\tbool operator ==(const Struct_" + str(n) + " &x) const\n\t{\n\t\treturn"
		for v in range(len(matches)):
			name = len(matches) == 2 and v == 0 and "first" \
					or len(matches) == 2 and v == 1 and "second" or "v" + str(v)
			code += " " + name + " == x." + name + " &&"
		code += " true;\n\t}\n};\n\n"

		return code

	# generate function for reading a struct
	def format2getStruct(self, n):
		matches = [x for x in map(lambda x: x, re.finditer(
				'(' + self.pttnType + ')(' + self.pttnWeakerSep + '*)', self.m.group("seq")))]

		code = "" """\
int get_""" + str(n) + """(istream &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
""" ""
		if self.m.group("pre"):
			code += "\tskip(in, \"" + self.m.group("pre") + "\");\n"

		for v in range(len(matches)):
			type = matches[v].group(1)
			stop = matches[v].group(2)
			name = len(matches) == 1 and "x" \
					or len(matches) == 2 and v == 0 and "x.first" \
					or len(matches) == 2 and v == 1 and "x.second" or "x.v" + str(v)

			# read int8_t/uint8_t as int instead of char
			if   type == "b":
				code += "\t{  int32_t b; if (!(in >> b)) return -1; " + name + " = b; }\n"
			elif type == "B":
				code += "\t{ uint32_t B; if (!(in >> B)) return -1; " + name + " = B; }\n"
			# read signed/unsigned char using get()
			elif type == "c" or type == "C":
				code += "\t{ " + name + " = in.get(); if (!in) return -1; }\n"
			# read UTF-16 as UTF-8 chars
			elif type == "u":
				code += "\tif (!getUtf16(in, " + name + ")) return -1;\n"
			# read other types using iostream
			else:
				code += "\tif (!(in >> " + name + ")) return -1;\n"
			if stop:
				code += "\tskip(in, \"" + stop + "\");\n"

		code += "" """
	if (files) append(files[0], x);

	return 0;
}

int get_""" + str(n) + """(const string &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
	const char *p = &*in.begin(), *p_next;

""" ""
		if self.m.group("pre"):
			code += "\tskip(p, \"" + self.m.group("pre") + "\");\n"

		for v in range(len(matches)):
			type = matches[v].group(1)
			stop = matches[v].group(2)
			name = len(matches) == 1 and "x" \
					or len(matches) == 2 and v == 0 and "x.first" \
					or len(matches) == 2 and v == 1 and "x.second" or "x.v" + str(v)

			# read signed/unsigned char
			if   type == "c" or type == "C":
				code += "\t\tif (p >= &*in.end()) return -1; " + name + " = *p ++;\n"
			# read UTF-16 as UTF-8 chars
			elif type == "u":
				code += "\t\tif (p >= &*in.end() || !getUtf16(p, " + name + ")) return -1;\n"
			# read integer and float using strtoxx
			else:
				if   type == "b" or type == "s" or type == "l":
					code += "\t\t" + name + " = strtol  (p, (char **)&p_next, 10);\n"
				elif type == "B" or type == "S" or type == "L":
					code += "\t\t" + name + " = strtoul (p, (char **)&p_next, 10);\n"
				elif type == "q":
					code += "\t\t" + name + " = strtoll (p, (char **)&p_next, 10);\n"
				elif type == "Q":
					code += "\t\t" + name + " = strtoull(p, (char **)&p_next, 10);\n"
				elif type == "f":
					code += "\t\t" + name + " = strtof  (p, (char **)&p_next);\n"
				elif type == "d":
					code += "\t\t" + name + " = strtod  (p, (char **)&p_next);\n"
				code += "\t\tif (p == p_next) return -1; p = p_next;\n"
			if stop:
				code += "\t\tskip(p, \"" + stop + "\");\n"

		code += "" """
	if (files) append(files[0], x);

	return 0;
}

""" ""
		return code

	# generate function for writing a struct
	def format2putStruct(self, n):
		matches = [x for x in map(lambda x: x, re.finditer(
				'(' + self.pttnType + ')(' + self.pttnWeakerSep + '*)', self.m.group("seq")))]

		code = "void put_" + str(n) \
				+ "(ostream &out, const " + self.type + " &x)\n{\n"

		if self.m.group("pre"):
			code += "\tout << \"" + self.m.group("pre") + "\";\n"

		for v in range(len(matches)):
			type = matches[v].group(1)
			stop = matches[v].group(2)
			name = len(matches) == 1 and "x" \
					or len(matches) == 2 and v == 0 and "x.first" \
					or len(matches) == 2 and v == 1 and "x.second" or "x.v" + str(v)

			# set proper floating point precision
			if   type == "f": code += "\tout.precision(numeric_limits<float >::digits10+2);\n"
			elif type == "d": code += "\tout.precision(numeric_limits<double>::digits10+2);\n"
			# write int8_t/uint8_t as int instead of char
			if   type == "b": code += "\tout << ( int32_t)" + name + ";\n"
			elif type == "B": code += "\tout << (uint32_t)" + name + ";\n"
			# write UTF-16 as UTF-8 chars
			elif type == "u": code += "\tputUtf16(out, " + name + ");\n"
			# write other types using iostream
			else:             code += "\tout << " + name + ";\n"
			if stop:          code += "\tout << \"" + stop + "\";\n"

		code += "}\n\n"

		return code

	# generate function for reading a struct sequence
	def format2getVectorStruct(self, n):
		matches = [x for x in map(lambda x: x, re.finditer(
				'(' + self.pttnType + ')(' + self.pttnWeakerSep + '*)', self.m.group("seq")))]

		code = "" """\
int get_""" + str(n) + """(istream &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
	typedef Container<""" + self.type + """>::std_value_type::value_type value_type;
	value_type v;
	x.resize(0);

	while (in)
	{
""" ""
		if self.m.group("pre"):
			code += "\t\tskip(in, \"" + self.m.group("pre") + "\");\n"

		for v in range(len(matches)):
			type = matches[v].group(1)
			stop = matches[v].group(2)
			name = len(matches) == 1 and "v" \
					or len(matches) == 2 and v == 0 and "v.first" \
					or len(matches) == 2 and v == 1 and "v.second" or "v.v" + str(v)

			# read int8_t/uint8_t as int instead of char
			if   type == "b":
				code += "\t\t{  int32_t b; if (!(in >> b)) continue; " + name + " = b; }\n"
			elif type == "B":
				code += "\t\t{ uint32_t B; if (!(in >> B)) continue; " + name + " = B; }\n"
			# read signed/unsigned char using get()
			elif type == "c" or type == "C":
				code += "\t\t{ " + name + " = in.get(); if (!in) continue; }\n"
			# read UTF-16 as UTF-8 chars
			elif type == "u":
				code += "\t\tif (!getUtf16(in, " + name + ")) continue;\n"
			# read other types using iostream
			else:
				code += "\t\tif (!(in >> " + name + ")) continue;\n"
			if stop:
				code += "\t\tskip(in, \"" + stop + "\");\n"

		code += "" """
		if (files) append(files[1], v);
		else x.push_back(v);
	}

	if (files) append<tell_helper<""" + self.type + """>::type>(
			files[0], tell<value_type>(files[1]));

	return 0;
}

int get_""" + str(n) + """(const string &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
	typedef Container<""" + self.type + """>::std_value_type::value_type value_type;
	value_type v;
	x.resize(0);

	const char *p_next = 0;

	for (const char *p = &*in.begin(); p < &*in.end(); )
	{
""" ""
		if self.m.group("pre"):
			code += "\t\tskip(p, \"" + self.m.group("pre") + "\");\n"

		for v in range(len(matches)):
			type = matches[v].group(1)
			stop = matches[v].group(2)
			name = len(matches) == 1 and "v" \
					or len(matches) == 2 and v == 0 and "v.first" \
					or len(matches) == 2 and v == 1 and "v.second" or "v.v" + str(v)

			# read signed/unsigned char
			if   type == "c" or type == "C":
				code += "\t\tif (p >= &*in.end()) break; " + name + " = *p ++;\n"
			# read UTF-16 as UTF-8 chars
			elif type == "u":
				code += "\t\tif (p >= &*in.end() || !getUtf16(p, " + name + ")) break;\n"
			# read integer and float using strtoxx
			else:
				if   type == "b" or type == "s" or type == "l":
					code += "\t\t" + name + " = strtol  (p, (char **)&p_next, 10);\n"
				elif type == "B" or type == "S" or type == "L":
					code += "\t\t" + name + " = strtoul (p, (char **)&p_next, 10);\n"
				elif type == "q":
					code += "\t\t" + name + " = strtoll (p, (char **)&p_next, 10);\n"
				elif type == "Q":
					code += "\t\t" + name + " = strtoull(p, (char **)&p_next, 10);\n"
				elif type == "f":
					code += "\t\t" + name + " = strtof  (p, (char **)&p_next);\n"
				elif type == "d":
					code += "\t\t" + name + " = strtod  (p, (char **)&p_next);\n"
				code += "\t\tif (p == p_next) break; p = p_next;\n"
			if stop:
				code += "\t\tskip(p, \"" + stop + "\");\n"

		code += "" """
		if (files) append(files[1], v);
		else x.push_back(v);
	}

	if (files) append<tell_helper<""" + self.type + """>::type>(
			files[0], tell<value_type>(files[1]));

	return 0;
}

""" ""
		return code

	# generate function for writing a struct sequence
	def format2putVectorStruct(self, n):
		matches = [x for x in map(lambda x: x, re.finditer(
				'(' + self.pttnType + ')(' + self.pttnWeakerSep + '*)', self.m.group("seq")))]

		code  = "template <class ContainerT>\n"
		code += "void put_" + str(n) + "(ostream &out, const ContainerT &x)\n{\n"

		code += "\tfor (size_t i = 0; i != x.size(); i ++)\n\t{\n"

		if self.m.group("pre"):
			code += "\t\tout << \"" + self.m.group("pre") + "\";\n"

		for v in range(len(matches)):
			type = matches[v].group(1)
			stop = matches[v].group(2)
			name = len(matches) == 1 and "x[i]" \
					or len(matches) == 2 and v == 0 and "x[i].first" \
					or len(matches) == 2 and v == 1 and "x[i].second" or "x[i].v" + str(v)

			# set proper floating point precision
			if   type == "f": code += "\t\tout.precision(numeric_limits<float >::digits10 + 2);\n"
			elif type == "d": code += "\t\tout.precision(numeric_limits<double>::digits10 + 2);\n"
			# write int8_t/uint8_t as int instead of char
			if   type == "b": code += "\t\tout << ( int32_t)" + name + ";\n"
			elif type == "B": code += "\t\tout << (uint32_t)" + name + ";\n"
			# write UTF-16 as UTF-8 chars
			elif type == "u": code += "\t\tputUtf16(out, " + name + ");\n"
			# write other types using iostream
			else:             code += "\t\tout << " + name + ";\n"
			if stop:          code += "\t\tout << \"" + stop + "\";\n"

		code += "\t}\n}\n\n"

		return code

	# generate function for reading a Vector
	def format2getVector(self, n):
		return "" """\
int get_""" + str(n) + """(istream &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
	string line;

	static const string sep    = """ + '\"' + self.m.group("sep"   ) + '\"' + """;

	typedef Container<""" + self.sub.type + """>::std_value_type value_type;
	value_type v;
	x.resize(0);

	while (getline(in, line, sep))
	{
""" + (self.m.group("sub").lower() in ["c*", "c**"] and " " or """\
""" + (hasattr(self.sub, "isStructOrSequence") and """\
		if (get_""" + str(n + 1) + """(line, v, files == 0 ? 0 : files + 1)) continue;
""" or """\
		istringstream isv(line);
		if (get_""" + str(n + 1) + """(isv,  v, files == 0 ? 0 : files + 1)) continue;
""")) + """\

		if (files)
		{
""" + (self.m.group("sub").lower() in ["c*", "c**"] and """\
			append(files[2], &line[0], &line[line.size()]);
			append<tell_helper<""" + self.sub.type + """>::type>(
					files[1], tell<char>(files[2]));
""" or " ") + """\
		}
		else
		{
""" + (self.m.group("sub").lower() in ["c*", "c**"] and """\
			x.push_back(value_type(
					(value_type::value_type *)&line[0],
					(value_type::value_type *)&line[line.size()]));
""" or """\
			x.push_back(v);
""") + """\
		}
	}

	if (files) append<tell_helper<""" + self.type + """>::type>(
			files[0], tell<""" + self.sub.type + """>(files[1]));

	return 0;
}

""" ""

	# generate function for writing a Vector
	def format2putVector(self, n):
		return "" """\
template <class ContainerT>
void put_""" + str(n) + """(ostream &out, const ContainerT &x)
{
	static const string sep    = """ + '\"' + self.m.group("sep"   ) + '\"' + """;

	for (size_t i = 0; i != x.size(); i ++)
	{
""" + (self.m.group("sub").lower() in ["c*", "c**"] and """\
		out << string((char *)&*x[i].begin(), (char *)&*x[i].end());
""" or """\
		put_""" + str(n + 1) + """(out, x[i]);
""") + """\
		out << sep;
	}
}

""" ""

	# generate function for reading a Trie<bool>
	def format2getTrieSet(self, n):
		return "" """\
int get_""" + str(n) + """(istream &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
	string line;

	static const string keysep = """ + '\"' + self.m.group("keysep") + '\"' + """;

	typedef Container<""" + self.key.type + """>::std_value_type key_type;
	key_type k;
	x.clear();

	while (getline(in, line, keysep))
	{
""" + (self.m.group("key").lower() in ["c*", "c**"] and " " or """\
""" + (hasattr(self.key, "isStructOrSequence") and """\
		if (get_""" + str(n + 1) + """(line, k, files == 0 ? 0 : files + 1)) continue;
""" or """\
		istringstream isk(line);
		if (get_""" + str(n + 1) + """(isk,  k, files == 0 ? 0 : files + 1)) continue;
""")) + """\

		if (files)
		{
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
			append(files[2], &line[0], &line[line.size()]);
			append<tell_helper<""" + self.key.type + """>::type>(
					files[1], tell<char>(files[2]));
""" or " ") + """\
			append(files[1 + """ + str(self.key.size) + """], true);
		}
		else
		{
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
			x.push_back(make_pair(key_type(
					(key_type::value_type *)&line[0],
					(key_type::value_type *)&line[line.size()]), true));
""" or """\
			x.push_back(make_pair(k, true));
""") + """\
		}
	}

	if (files) append<tell_helper<""" + self.type + """>::type>(
			files[0], tell<""" + self.key.type + """>(files[1]));

	return 0;
}

""" ""

	# generate function for writing a Trie<bool>
	def format2putTrieSet(self, n):
		return "" """\
template <class ContainerT>
void put_""" + str(n) + """(ostream &out, const ContainerT &x)
{
	static const string keysep = """ + '\"' + self.m.group("keysep") + '\"' + """;

	for (size_t i = 0; i != x.size(); i ++)
	{
""" + (self.type[0] == 'T' and """\
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
		out << x.template key<string>(x.begin() + i);
""" or """\
		put_""" + str(n + 1) + """(out, x.template key<Container<
				""" + self.key.type + """>::std_value_type>(x.begin() + i));
""") + """\
""" or """\
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
		out << string(
				(char *)(x.begin() + i)->first.begin(),
				(char *)(x.begin() + i)->first.end());
""" or """\
		put_""" + str(n + 1) + """(out, (x.begin() + i)->first);
""") + """\
""") + """\
		out << keysep;
	}
}

""" ""

	# generate function for reading a Trie
	def format2getTrie(self, n):
		return "" """\
int get_""" + str(n) + """(istream &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
	string line;

	static const string keysep = """ + '\"' + self.m.group("keysep") + '\"' + """;
	static const string sep    = """ + '\"' + self.m.group("sep"   ) + '\"' + """;

	typedef Container<""" + self.sub.type + """>::std_value_type value_type;
	typedef Container<""" + self.key.type + """>::std_value_type key_type;
	key_type k;
	value_type v;
	x.clear();

	while (getline(in, line, sep))
	{
		size_t t = line.find(keysep);
		if (t == string::npos) continue;

""" + (self.m.group("key").lower() in ["c*", "c**"] and " " or """\
""" + (hasattr(self.key, "isStructOrSequence") and """\
		if (get_""" + str(n + 1) + """(line.substr(0, t), k, files == 0 ? 0 : files + 1)) continue;
""" or """\
		istringstream isk(line.substr(0, t));
		if (get_""" + str(n + 1) + """(isk, k, files == 0 ? 0 : files + 1)) continue;
""")) + """\

""" + (self.m.group("sub").lower() in ["c*", "c**"] and " " or """\
""" + (hasattr(self.sub, "isStructOrSequence") and """\
		if (get_""" + str(n + 1 + self.key.size) + """(line.substr(t + keysep.size()), v, files == 0 ? 0 :
				files + """ + str(1 + self.key.size) + """)) continue;
""" or """\
		istringstream isv(line.substr(t + keysep.size()));
		if (get_""" + str(n + 1 + self.key.size) + """(isv, v, files == 0 ? 0 :
				files + """ + str(1 + self.key.size) + """)) continue;
""")) + """\

		if (files)
		{
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
			append(files[2], &line[0], &line[t]);
			append<tell_helper<""" + self.key.type + """>::type>(
					files[1], tell<char>(files[2]));
""" or " ") + """\

""" + (self.m.group("sub").lower() in ["c*", "c**"] and """\
			append(files[2 + """ + str(self.key.size) + """],
					&line[t + keysep.size()], &line[line.size()]);
			append<tell_helper<""" + self.sub.type + """>::type>(
					files[1 + """ + str(self.key.size) + """],
					tell<char>(files[2 + """ + str(self.key.size) + """]));
""" or " ") + """\
		}
		else
		{
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
			x.push_back(make_pair(key_type(
					(key_type::value_type *)&line[0],
					(key_type::value_type *)&line[t]),
""" or """\
			x.push_back(make_pair(k,
""") + """\

""" + (self.m.group("sub").lower() in ["c*", "c**"] and """\
					value_type(
						(value_type::value_type *)&line[t + keysep.size()],
						(value_type::value_type *)&line[line.size()])));
""" or """\
					v));
""") + """\
		}
	}

	if (files) append<tell_helper<""" + self.type + """>::type>(
			files[0], tell<""" + self.key.type + """>(files[1]));

	return 0;
}

""" ""

	# generate function for writing a Trie
	def format2putTrie(self, n):
		return "" """\
template <class ContainerT>
void put_""" + str(n) + """(ostream &out, const ContainerT &x)
{
	static const string keysep = """ + '\"' + self.m.group("keysep") + '\"' + """;
	static const string sep    = """ + '\"' + self.m.group("sep"   ) + '\"' + """;

	for (size_t i = 0; i != x.size(); i ++)
	{
""" + (self.type[0] == 'T' and """\
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
		out << x.template key<string>(x.begin() + i);
""" or """\
		put_""" + str(n + 1) + """(out, x.template key<Container<
				""" + self.key.type + """>::std_value_type>(x.begin() + i));
""") + """\
""" or """\
""" + (self.m.group("key").lower() in ["c*", "c**"] and """\
		out << string(
				(char *)(x.begin() + i)->first.begin(),
				(char *)(x.begin() + i)->first.end());
""" or """\
		put_""" + str(n + 1) + """(out, (x.begin() + i)->first);
""") + """\
""") + """\
		out << keysep;

""" + (self.type[0] == 'T' and """\
""" + (self.m.group("sub").lower() in ["c*", "c**"] and """\
		out << string(
				(char *)(x.begin() + i)->begin(), (char *)(x.begin() + i)->end());
""" or """\
		put_""" + str(n + 1 + self.key.size) + """(out, *(x.begin() + i));
""") + """\
""" or """\
""" + (self.m.group("sub").lower() in ["c*", "c**"] and """\
		out << string(
				(char *)(x.begin() + i)->second.begin(),
				(char *)(x.begin() + i)->second.end());
""" or """\
		put_""" + str(n + 1 + self.key.size) + """(out, (x.begin() + i)->second);
""") + """\
""") + """\
		out << sep;
	}
}

""" ""

	# generate function for reading a Pair
	def format2getPair(self, n):
		return "" """\
int get_""" + str(n) + """(istream &in,
		Container<""" + self.type + """>::std_value_type &x, fstream *files = 0)
{
	string line((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

	static const string sep    = """ + '\"' + self.m.group("sep"   ) + '\"' + """;

	typedef Container<""" + self.sub1.type + """>::std_value_type first_type;
	typedef Container<""" + self.sub2.type + """>::std_value_type second_type;

	size_t t = line.find(sep);
	if (t == string::npos) return -1;

""" + (self.m.group("sub1").lower() in ["c*", "c**"] and """\
	if (files)
	{
		append(files[1], (char *)&line[0], (char *)&line[t]);
		append<tell_helper<""" + self.sub1.type + """>::type>(
				files[0], tell<char>(files[1]));
	}
	else x.first = first_type(
			(first_type::value_type *)&line[0],
			(first_type::value_type *)&line[t]);
""" or """\
""" + (hasattr(self.sub1, "isStructOrSequence") and """\
	if (get_""" + str(n) + """(line.substr(0, t), x.first, files == 0 ? 0 : files)) return -1;
""" or """\
	istringstream isv1(line.substr(0, t));
	if (get_""" + str(n) + """(isv1, x.first, files == 0 ? 0 : files)) return -1;
""")) + """\

""" + (self.m.group("sub2").lower() in ["c*", "c**"] and """\
	if (files)
	{
		append(files[1 + """ + str(self.sub1.size) + """],
				(char *)&line[t + sep.size()], (char *)&line[line.size()]);
		append<tell_helper<""" + self.sub2.type + """>::type>(
				files[0 + """ + str(self.sub1.size) + """],
				tell<char>(files[1 + """ + str(self.sub1.size) + """]));
	}
	else x.second = second_type(
			(second_type::value_type *)&line[t + sep.size()],
			(second_type::value_type *)&line[line.size()]);
""" or """\
""" + (hasattr(self.sub2, "isStructOrSequence") and """\
	if (get_""" + str(n + self.sub1.size) + """(line.substr(t + sep.size()), x.second, files == 0 ? 0 :
			files + """ + str(self.sub1.size) + """)) return -1;
""" or """\
	istringstream isv2(line.substr(t + sep.size()));
	if (get_""" + str(n + self.sub1.size) + """(isv2, x.second, files == 0 ? 0 :
			files + """ + str(self.sub1.size) + """)) return -1;
""")) + """\

	return 0;
}

""" ""

	# generate function for writing a Pair
	def format2putPair(self, n):
		return "" """\
void put_""" + str(n) + """(ostream &out,
		const Pair<""" + self.sub1.type + """, """ + self.sub2.type + """> &x)
{
	static const string sep    = """ + '\"' + self.m.group("sep"   ) + '\"' + """;

""" + (self.m.group("sub1").lower() in ["c*", "c**"] and """\
	out << string((char *)&*x.first.begin(), (char *)&*x.first.end());
""" or """\
	put_""" + str(n) + """(out, x.first);
""") + """\
	out << sep;

""" + (self.m.group("sub2").lower() in ["c*", "c**"] and """\
	out << string((char *)&*x.second.begin(), (char *)&*x.second.end());
""" or """\
	put_""" + str(n + self.sub1.size) + """(out, x.second);
""") + """\
}

""" ""

container = Container(0, options.format)
container_input = options.index   and Container(2000, options.index)   or \
									options.inverse and Container(2000, options.inverse) or None

# generate C++ source code

cpp = "" """\
// generated by fasttrie.py -f '""" + options.format + """'""" + ( \
		options.index   and " -i '" + options.index   + "'" or \
		options.inverse and " -r '" + options.inverse + "'" or "") + """

#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>

#include "FastTrie.h"
#include "GeneralMap.h"

""" + "\n".join(map(lambda x: '#include \"' + x + '\"', options.extend)) + """\

using namespace std;
using namespace ft2;
using namespace generalmap;

// common utility functions

const vector<string> split(const string &delimiter, const string &source)
{
	if (delimiter.empty()) return vector<string>(1, source);

	vector<string> result;

	size_t prev_pos = 0;
	size_t pos = source.find(delimiter, 0);
	while (pos != string::npos)
	{
		result.push_back(source.substr(prev_pos, pos - prev_pos));
		pos += delimiter.size();
		prev_pos = pos;
		pos = source.find(delimiter, pos);
	}
	result.push_back(source.substr(prev_pos));

	return result;
}

istream &getUtf16(istream &in, uint16_t &utf16)
{
	const uint16_t replacementChar = 0xFFFD;

	uint16_t result = (uint8_t)in.get(); if (!in) return in;

	if      (result < 0x80) ;
	else if (result < 0xC2) // unexpected continuation byte or overlong
		result = replacementChar;
	else if (result < 0xE0) // 2 bytes
	{
		result = (result & 0x1F) << 6;

		uint16_t next = (uint8_t)in.get(); if (!in) return in;
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return in; }
		result |= (next & 0x3F);
	}
	else if (result < 0xF0) // 3 bytes
	{
		result = (result & 0xF) << 12;

		uint16_t next = (uint8_t)in.get(); if (!in) return in;
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return in; }
		result |= (next & 0x3F) << 6;

		next          = (uint8_t)in.get(); if (!in) return in;
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return in; }
		result |= (next & 0x3F);

		if (result < 0x800) result = replacementChar; // overlong
	}
	else if (result < 0xF8) // 4 bytes
	{
		uint16_t next = (uint8_t)in.get(); if (!in) return in;
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return in; }
		next          = (uint8_t)in.get(); if (!in) return in;
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return in; }
		next          = (uint8_t)in.get(); if (!in) return in;
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return in; }

		result = replacementChar; // not in UCS-2
	}
	else
		result = replacementChar; // not in UTF-8 standard

	utf16 = result; return in;
}

bool getUtf16(const char *&in, uint16_t &utf16)
{
	const uint16_t replacementChar = 0xFFFD;

	uint16_t result = (uint8_t)(*in ++);

	if      (result < 0x80) ;
	else if (result < 0xC2) // unexpected continuation byte or overlong
		result = replacementChar;
	else if (result < 0xE0) // 2 bytes
	{
		result = (result & 0x1F) << 6;

		if (!*in) return false; uint16_t next = (uint8_t)(*in ++);
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return true; }
		result |= (next & 0x3F);
	}
	else if (result < 0xF0) // 3 bytes
	{
		result = (result & 0xF) << 12;

		if (!*in) return false; uint16_t next = (uint8_t)(*in ++);
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return true; }
		result |= (next & 0x3F) << 6;

		if (!*in) return false;          next = (uint8_t)(*in ++);
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return true; }
		result |= (next & 0x3F);

		if (result < 0x800) result = replacementChar; // overlong
	}
	else if (result < 0xF8) // 4 bytes
	{
		if (!*in) return false; uint16_t next = (uint8_t)(*in ++);
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return true; }
		if (!*in) return false;          next = (uint8_t)(*in ++);
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return true; }
		if (!*in) return false;          next = (uint8_t)(*in ++);
		if (next < 0x80 || next >= 0xC0) { utf16 = replacementChar; return true; }

		result = replacementChar; // not in UCS-2
	}
	else
		result = replacementChar; // not in UTF-8 standard

	utf16 = result; return true;
}

ostream &putUtf16(ostream &out, uint16_t utf16)
{
	if      (utf16 < 0x80 )
		out << (char)(utf16);
	else if (utf16 < 0x800)
		out << (char)(utf16 >>  6 | 0xC0) << (char)((utf16 & 0x3F) | 0x80);
	else
		out << (char)( utf16 >> 12         | 0xE0)
				<< (char)((utf16 >>  6 & 0x3F) | 0x80)
				<< (char)((utf16       & 0x3F) | 0x80);

	return out;
}

istream &getline(istream &in, string &line, const string &delimiter)
{
	if (delimiter.empty()) return getline(in, line);

	size_t size_1 = delimiter.size() - 1;

	if (getline(in, line, delimiter[size_1]) && size_1)
	{
		string next;

		while (!in.eof() && (line.size() < size_1 || strncmp(
				delimiter.c_str(), line.c_str() + line.size() - size_1, size_1)))
		{
			line += delimiter[size_1];
			if (getline(in, next, delimiter[size_1])) line += next;
		}
		if (!in.eof()) line.resize(line.size() - size_1);

		in.clear();
	}

	return in;
}

inline void skip(istream &in, const char *separator)
{
	for (const char *p = separator; *p; p ++)
		if (in.get() != *p) { in.unget(); break; }
}

inline void skip(const char *&in, const char *separator)
{
	while (*separator && *in == *separator) { in ++; separator ++; }
}

template <class T>
struct tell_helper { typedef T type; };
template <class ValueT, class SizeT>
struct tell_helper<Vector<ValueT, SizeT> > { typedef SizeT type; };
template <class KeyT, class ValueT, int option, class CharT, class SizeT>
struct tell_helper<Trie<KeyT, ValueT, option, CharT, SizeT> >
{ typedef size_t type; };
template <class KeyT, class ValueT, class HashT, class SizeT>
struct tell_helper<HashMap<KeyT, ValueT, HashT, SizeT> >
{ typedef size_t type; };
template <class ValueT1, class ValueT2>
struct tell_helper<Pair<ValueT1, ValueT2> >
{ typedef typename tell_helper<ValueT1>::type type; };

template <class T>
inline size_t tell(fstream &out)
{
	return out.tellp() / sizeof(typename tell_helper<T>::type);
}

template <class T>
inline void append(fstream &out, const T &value)
{
	out.write((char *)&value, sizeof(T));
}

template <class T>
inline void append(fstream &out, const T *begin, const T *end)
{
	out.write((char *)begin, (char *)end - (char *)begin);
}

template <class T> struct cat_helper;

template <class T>
fstream *cat(fstream *files, ostream &out)
{
	return cat_helper<T>::cat(files, out);
}

template <class T>
struct cat_helper
{
	static fstream *cat(fstream *files, ostream &out)
	{
		files[0].seekg(0, ios::end);
		size_t numValues = files[0].tellg() / sizeof(T);
		size_t bytes = sizeof(T) * numValues;
		bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;
		files[0].seekg(0, ios::beg);

		char zeros[FT_ALIGN] = { 0 };

		out.write((char *)&numValues, sizeof(numValues));
		out.write(zeros,   FT_ALIGN - sizeof(numValues));
		if (numValues) out << files[0].rdbuf();
		out.write(zeros, bytes);

		return files + 1;
	}
};

template <class ValueT, class SizeT>
struct cat_helper<Vector<ValueT, SizeT> >
{
	static fstream *cat(fstream *files, ostream &out)
	{
		files[0].seekg(0, ios::end);
		size_t numEntries = files[0].tellg() / sizeof(SizeT) + 1;
		size_t bytes = sizeof(SizeT) * numEntries;
		bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;
		files[0].seekg(0, ios::beg);

		char zeros[FT_ALIGN] = { 0 };

		out.write((char *)&numEntries, sizeof(numEntries));
		out.write(zeros,    FT_ALIGN - sizeof(numEntries));
		out.write(zeros, sizeof(SizeT)); // write entries[0]
		if (numEntries > 1) out << files[0].rdbuf();
		out.write(zeros, bytes);

		return ::cat<ValueT>(files + 1, out);
	}
};

template <class ValueT1, class ValueT2>
struct cat_helper<Pair<ValueT1, ValueT2> >
{
	static fstream *cat(fstream *files, ostream &out)
	{
		files = ::cat<ValueT1>(files, out);
		files = ::cat<ValueT2>(files, out);

		return files;
	}
};

template <class T>
struct Compare
{
	bool operator ()(T a, T b) const { return
			a.first         < b.first         || (a.first         == b.first && (
			a.second.second < b.second.second || (a.second.second == b.second.second &&
			a.second.first  < b.second.first))); }
};

// generated structs and functions

""" + container.code + """\
""" + ((options.index or options.inverse) and container_input.code or "") + """\

int main(int argc, char **argv)
{
	string tmpdir;
	bool printing = false;
	bool index    = false;
	bool inverse  = false;
	bool last     = false;

	vector<char *> args(argv + 1, argv + argc);

	while (!last && !args.empty() && args[0][0] == '-')
	{
		if (args[0] == string("-d") && args.size() > 1)
		{
			tmpdir = args[1];
			args.erase(args.begin());
		}
		else if (args[0] == string("-p")) printing = true;
		else if (args[0] == string("-i")) index    = true;
		else if (args[0] == string("-r")) inverse  = true;
		else if (args[0] == string("--")) last     = true;

		args.erase(args.begin());
	}

	const char *tmpdir_c = tmpdir.empty() ? 0 : tmpdir.c_str();

	if (args.empty())
	{
		if (!tmpdir.empty())
		{
			size_t numContainers = """ + str(container.size) + """;

			fstream *files = new fstream[numContainers];
			ofstream fileCat((tmpdir + "/cat").c_str());

			for (size_t i = 0; i != numContainers; i ++)
			{
				char n[16]; sprintf(n, "%d", (int)i);
				files[i].open((tmpdir + "/" + n).c_str(), ios::in | ios::out | ios::trunc);
			}

			Container<""" + container.type + """>::std_value_type std_container;

			get_0(cin, std_container, files);

			cat<""" + container.diskType + """>(files, """ + (
					container.type != container.diskType and "fileCat" or "cout") + """);

			delete []files; fileCat.close();

			for (size_t i = 0; i != numContainers; i ++)
			{
				char n[16]; sprintf(n, "%d", (int)i);
				unlink((tmpdir + "/" + n).c_str());
			}

""" + (container.type != container.diskType and """\
			Container<""" + container.diskType + """> input((tmpdir + "/cat").c_str());

			Container<""" + container.type + """>::build(
					ostreambuf_iterator<char>(cout), input.begin(), input.end(), tmpdir_c);
""" or " ") + """\
		}
		else
		{
			Container<""" + container.type + """>::std_value_type std_container;

			get_0(cin, std_container);

			Container<""" + container.type + """>::build(
					ostreambuf_iterator<char>(cout), &std_container, &std_container + 1);
		}
	}
""" + ("key" in container.m.groupdict() and """\
	else if (printing && !args.empty())
	{
		static const string sep = """ + '\"' + container.m.group("sep"
				in container.m.groupdict() and "sep" or "keysep") + '\"' + """;

		Container<""" + container.type + """> container(args[0]);

		Container<""" + container.key.type + """>::std_value_type k;

		string line;
		while (getline(cin, line, sep))
		{
			istringstream isk(line);
			if (get_1(isk, k) == 0)
			{
""" + ("sub" not in container.m.groupdict() and """\
				cout << container[0](k);
""" or """\
				put_""" + str(1 + container.key.size) + """(cout, container[0](k));
""") + """\
				cout << sep;
			}
		}
	}
""" or " ") + """\
""" + (options.index and """\
	else if (index && !args.empty())
	{
		typedef pair<
				General<""" + container.type     + """>::key_type,
				General<""" + container.sub.type + """>::value_type> KeyIndex;

		Container<""" + container_input.type + """> container_input(args[0]);

		vector<KeyIndex, TmpAlloc<KeyIndex> > indices(tmpdir_c);

		size_t size = 0;
		for (""" + container_input.type + """::const_iterator
				it = container_input[0].begin(); it != container_input[0].end(); ++ it)
			size += general(container_input[0])[it].size();
		indices.reserve(size);
		for (""" + container_input.type + """::const_iterator
				it = container_input[0].begin(); it != container_input[0].end(); ++ it)
			for (""" + container_input.sub.type + """::const_iterator
					itSub  = general(container_input[0])[it].begin();
					itSub != general(container_input[0])[it].end(); ++ itSub)
				indices.push_back(make_pair(
						general(general(container_input[0])[it])[itSub],
						general(container_input[0]).key(it)));

		sort(indices.begin(), indices.end());

		if (!tmpdir.empty())
		{
			typedef Conditional<
					General<""" + container.diskType + """>::hasSecond, pair<
					General<""" + container.diskType + """>::key_type,
					General<""" + container.diskType + """>::value_type>,
					General<""" + container.diskType + """>::value_type>::type KeyRange;

			vector<KeyRange, TmpAlloc<KeyRange> > cps_container(tmpdir_c);

			size_t numContainers = """ + str(container.sub.size) + """;

			fstream *files = new fstream[numContainers];
			ofstream fileCat((tmpdir + "/cat").c_str());

			for (size_t i = 0; i != numContainers; i ++)
			{
				char n[16]; sprintf(n, "%d", (int)i);
				files[i].open((tmpdir + "/" + n).c_str(), ios::in | ios::out | ios::trunc);
			}

			for (size_t i = 0, last_i = 0; i != indices.size(); i ++)
			{
""" + (container.sub.size == 2 and """\
				append(files[1], indices[i].second);
""" or """\
				append(files[2], indices[i].second.begin(), indices[i].second.end());
				append<tell_helper<""" + container.sub.sub.type + """>::type>(files[1],
						tell<""" + container.sub.sub.type + """::value_type>(files[2]));
""") + """\

				if (i == indices.size() - 1 || indices[i].first != indices[i + 1].first)
				{
					if (!General<""" + container.type + """>::hasSecond)
					{
""" + (container.type[0] == 'V' and container.sub.type[0] != 'P' and """\
						for (size_t j = cps_container.size(); j != indices[i].first; j ++)
							append<tell_helper<""" + container.sub.type + """>::type>(files[0], last_i);
""" or " ") + """\
					}
					append<tell_helper<""" + container.sub.type + """>::type>(files[0], i + 1);
					general(cps_container).insert(indices[i].first, """ + container.sub.diskType + """());

					last_i = i + 1;
				}
			}

			vector<KeyIndex, TmpAlloc<KeyIndex> >(tmpdir_c).swap(indices);

			cat<""" + container.sub.diskType + """>(files, fileCat);

			delete []files; fileCat.close();

			for (size_t i = 0; i != numContainers; i ++)
			{
				char n[16]; sprintf(n, "%d", (int)i);
				unlink((tmpdir + "/" + n).c_str());
			}

			Container<""" + container.sub.diskType + """> input((tmpdir + "/cat").c_str());

			for (size_t j = 0; j != cps_container.size(); j ++)
				general(cps_container)[cps_container.begin() + j] = input[j];

			Container<""" + container.type + """>::build(
					ostreambuf_iterator<char>(cout), &cps_container, &cps_container + 1, tmpdir_c);
		}
		else
		{
			Container<""" + container.type + """>::std_value_type std_container;

			for (size_t i = 0, last_i = 0; i != indices.size(); i ++)
				if (i == indices.size() - 1 || indices[i].first != indices[i + 1].first)
				{
					general(std_container).insert(indices[i].first, General<Container<
							""" + container.type + """>::std_value_type>::value_type());
					general(std_container)[general(std_container).find(indices[i].first)]
							.reserve(i + 1 - last_i);

					last_i = i + 1;
				}

			for (size_t i = 0; i != indices.size(); i ++)
				general(std_container)[general(std_container).find(indices[i].first)]
						.push_back(indices[i].second);

			vector<KeyIndex, TmpAlloc<KeyIndex> >(tmpdir_c).swap(indices);

			Container<""" + container.type + """>::build(
					ostreambuf_iterator<char>(cout), &std_container, &std_container + 1);
		}
	}
""" or " ") + """\
""" + (options.inverse and """\
	else if (inverse && !args.empty())
	{
		typedef pair<
				General<""" + container.type     + """>::key_type, pair<
				General<""" + container.sub.type + """>::key_type,
				General<""" + container.sub.type + """>::value_type> > KeyIndex;

		Container<""" + container_input.type + """> container_input(args[0]);

		vector<KeyIndex, TmpAlloc<KeyIndex> > indices(tmpdir_c);

		size_t size = 0;
		for (""" + container_input.type + """::const_iterator
				it = container_input[0].begin(); it != container_input[0].end(); ++ it)
			size += general(container_input[0])[it].size();
		indices.reserve(size);
		for (""" + container_input.type + """::const_iterator
				it = container_input[0].begin(); it != container_input[0].end(); ++ it)
			for (""" + container_input.sub.type + """::const_iterator
					itSub  = general(container_input[0])[it].begin();
					itSub != general(container_input[0])[it].end(); ++ itSub)
				indices.push_back(make_pair(
						general(general(container_input[0])[it]).key(itSub), make_pair(
						general(container_input[0]).key(it),
						general(general(container_input[0])[it])[itSub])));

		if (!General<""" + container.sub.type + """>::hasSecond)
			sort(indices.begin(), indices.end());
		else
			sort(indices.begin(), indices.end(), Compare<KeyIndex>());

		if (!tmpdir.empty())
		{
			typedef Conditional<
					General<""" + container.diskType + """>::hasSecond, pair<
					General<""" + container.diskType + """>::key_type,
					General<""" + container.diskType + """>::value_type>,
					General<""" + container.diskType + """>::value_type>::type KeyRange;

			vector<KeyRange, TmpAlloc<KeyRange> > cps_container(tmpdir_c);

			size_t numContainers = """ + str(container.sub.size) + """;

			fstream *files = new fstream[numContainers];
			ofstream fileCat((tmpdir + "/cat").c_str());

			for (size_t i = 0; i != numContainers; i ++)
			{
				char n[16]; sprintf(n, "%d", (int)i);
				files[i].open((tmpdir + "/" + n).c_str(), ios::in | ios::out | ios::trunc);
			}

			for (size_t i = 0, last_i = 0; i != indices.size(); i ++)
			{
				if (!General<""" + container.sub.type + """>::hasSecond)
				{
""" + (container.sub.type[0] == 'V' and (container.sub.size == 2 or container.sub.sub.type[0] == 'V') and """\
""" + (container.sub.size == 2 and """\
					for (size_t j = i > last_i ? indices[i - 1].second.first + 1 : 0;
							j != indices[i].second.first ; j ++)
						append(files[1], """ + container.sub.type + """::value_type());
					append(files[1], indices[i].second.second);
""" or """\
					for (size_t j = i > last_i ? indices[i - 1].second.first + 1 : 0;
							j != indices[i].second.first ; j ++)
						append<tell_helper<""" + container.sub.sub.type + """>::type>(files[1],
								tell<""" + container.sub.sub.type + """::value_type>(files[2]));
					append(files[2], indices[i].second.second.begin(), indices[i].second.second.end());
					append<tell_helper<""" + container.sub.sub.type + """>::type>(files[1],
							tell<""" + container.sub.sub.type + """::value_type>(files[2]));
""") + """\
""" or " ") + """\
				}
				else
				{
""" + (container.sub.size == 2 and """\
					append(files[1], indices[i].second);
""" or container.sub.type[0] == 'H' and """\
""" + (container.sub.key.size == 1 and """\
					append(files[1], indices[i].second.first);
""" or """\
					append(files[2], indices[i].second.first.begin(), indices[i].second.first.end());
					append<tell_helper<""" + container.sub.key.type + """>::type>(files[1],
							tell<""" + container.sub.key.type + """::value_type>(files[2]));
""") + """\
					size_t b = 1 + """ + str(container.sub.key.size) + """;
""" + (container.sub.sub.size == 1 and """\
					append(files[b], indices[i].second.second);
""" or """\
					append(files[b + 1], indices[i].second.second.begin(), indices[i].second.second.end());
					append<tell_helper<""" + container.sub.sub.type + """>::type>(files[b],
							tell<""" + container.sub.sub.type + """::value_type>(files[b + 1]));
""") + """\
""" or container.sub.sub.type[0] == 'P' and """\
""" + (container.sub.sub.sub1.size == 1 and """\
					append(files[1], indices[i].second.first);
""" or """\
					append(files[2], indices[i].second.first.begin(), indices[i].second.first.end());
					append<tell_helper<""" + container.sub.sub.sub1.type + """>::type>(files[1],
							tell<""" + container.sub.sub.sub1.type + """::value_type>(files[2]));
""") + """\
					size_t b = 1 + """ + str(container.sub.sub.sub1.size) + """;
""" + (container.sub.sub.sub2.size == 1 and """\
					append(files[b], indices[i].second.second);
""" or """\
					append(files[b + 1], indices[i].second.second.begin(), indices[i].second.second.end());
					append<tell_helper<""" + container.sub.sub.sub2.type + """>::type>(files[b],
							tell<""" + container.sub.sub.sub2.type + """::value_type>(files[b + 1]));
""") + """\
""" or " ") + """\
				}

				if (i == indices.size() - 1 || indices[i].first != indices[i + 1].first)
				{
					if (!General<""" + container.type + """>::hasSecond)
					{
""" + (container.type[0] == 'V' and container.sub.type[0] != 'P' and """\
						for (size_t j = cps_container.size(); j != indices[i].first; j ++)
							append<tell_helper<""" + container.sub.type + """>::type>(files[0], last_i);
""" or " ") + """\
					}
					append<tell_helper<""" + container.sub.type + """>::type>(files[0], i + 1);
					general(cps_container).insert(indices[i].first, """ + container.sub.diskType + """());

					last_i = i + 1;
				}
			}

			vector<KeyIndex, TmpAlloc<KeyIndex> >(tmpdir_c).swap(indices);

			cat<""" + container.sub.diskType + """>(files, fileCat);

			delete []files; fileCat.close();

			for (size_t i = 0; i != numContainers; i ++)
			{
				char n[16]; sprintf(n, "%d", (int)i);
				unlink((tmpdir + "/" + n).c_str());
			}

			Container<""" + container.sub.diskType + """> input((tmpdir + "/cat").c_str());

			for (size_t j = 0; j != cps_container.size(); j ++)
				general(cps_container)[cps_container.begin() + j] = input[j];

			Container<""" + container.type + """>::build(
					ostreambuf_iterator<char>(cout), &cps_container, &cps_container + 1, tmpdir_c);
		}
		else
		{
			Container<""" + container.type + """>::std_value_type std_container;

			for (size_t i = 0, last_i = 0; i != indices.size(); i ++)
				if (i == indices.size() - 1 || indices[i].first != indices[i + 1].first)
				{
					general(std_container).insert(indices[i].first, General<Container<
							""" + container.type + """>::std_value_type>::value_type());
					general(std_container)[general(std_container).find(indices[i].first)]
							.reserve(i + 1 - last_i);

					last_i = i + 1;
				}

			for (size_t i = 0; i != indices.size(); i ++)
				general(general(std_container)[general(std_container).find(indices[i].first)])
						.insert(indices[i].second.first, indices[i].second.second);

			vector<KeyIndex, TmpAlloc<KeyIndex> >(tmpdir_c).swap(indices);

			Container<""" + container.type + """>::build(
					ostreambuf_iterator<char>(cout), &std_container, &std_container + 1);
		}
	}
""" or " ") + """\
	else for (size_t i = 0; i != args.size(); i ++)
	{
		Container<""" + container.type + """> container(args[i]);

		put_0(cout, container[0]);
	}

	return 0;
}
""" ""

tmpdir = tempfile.mkdtemp()

if not options.compile and os.getenv("FASTTRIE_COMPILE"):
	options.compile = os.getenv("FASTTRIE_COMPILE")
if options.compile:
	if sys.version_info >= (2, 5):
		exe = options.compile + "/" + hashlib.md5(( \
				options.format + options.index + options.inverse).encode('utf-8')).hexdigest()
	else:
		exe = options.compile + "/" + md5.new( \
				options.format + options.index + options.inverse).hexdigest()
	try:
		if not os.path.exists(options.compile):
			os.makedirs(options.compile)
		if not os.path.exists(exe + ".cpp") or not os.stat(exe + ".cpp").st_size:
			open(exe + ".cpp", "w").write(cpp)
	except: pass
else:
	exe = tmpdir + "/fasttrie"

def kill_handler(signum, frame):
	raise KeyboardInterrupt # treat kill as KeyboardInterrupt

signal.signal(signal.SIGTERM, kill_handler)

# compile and run

try:
	if sys.version_info >= (2, 4):
		if not os.access(exe, os.X_OK) or not os.stat(exe).st_size:
			p = subprocess.Popen(["g++", "-x", "c++", "-o", exe, "-O3", "-"]
					+ (options.include and ["-I", options.include] or [])
					+ (os.path.dirname(sys.argv[0])
								and ["-I", os.path.dirname(sys.argv[0])] or []),
					stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			(out, err) = p.communicate(input=cpp.encode('utf-8'))
			if out or err or \
					not os.access(exe, os.X_OK) or not os.stat(exe).st_size:
				raise Exception("error", "compile or execute failed")

		p = subprocess.Popen([exe]
				+ (options.disk     and ["-d", tmpdir] or [])
				+ (options.printing and ["-p"        ] or [])
				+ (options.index    and ["-i"        ] or [])
				+ (options.inverse  and ["-r"        ] or []) + ["--"] + args)
		p.wait()
	else:
		if not os.access(exe, os.X_OK) or not os.stat(exe).st_size:
			(out, input, err) = popen2.popen3("g++ -x c++ -o '" + exe + "' -O3 -"
					+ (options.include and " -I '" + options.include + "'" or "")
					+ (os.path.dirname(sys.argv[0])
								and " -I '" + os.path.dirname(sys.argv[0]) + "'" or ""))
			input.write(cpp)
			input.close()
			out = out.read()
			err = err.read()
			if out or err or \
					not os.access(exe, os.X_OK) or not os.stat(exe).st_size:
				raise Exception("error", "compile or execute failed")

		(out, input, err) = popen2.popen3("'" + exe + "' "
				+ (options.disk     and "-d '" + tmpdir + "' " or "")
				+ (options.printing and "-p "                  or "")
				+ (options.index    and "-i "                  or "")
				+ (options.inverse  and "-r "                  or "")
				+ "-- " + " ".join(map(lambda x: "'" + x + "'", args)))
		if not args:
			for line in sys.stdin:
				input.write(line)
		elif args and options.printing:
			while True:
				line = sys.stdin.readline()
				if not line: break
				input.write(line)
				input.flush()
				sys.stdout.write(out.readline())
				sys.stdout.flush()
		input.close()
		data = out.read(1048576)
		while data:
			sys.stdout.write(data)
			data = out.read(1048576)

except KeyboardInterrupt: pass
except:
	shutil.rmtree(tmpdir)
	if not "err" in locals():      err = ""
	if sys.version_info >= (3, 0):
		if   isinstance(err, bytes): err = err.decode('utf-8')
	if not isinstance(err, str):   err = err.read()
	sys.stderr.write(err)
	sys.stderr.write(os.path.basename(sys.argv[0]) + ": compile or execute failed\n")
	sys.exit(-1)

shutil.rmtree(tmpdir)
