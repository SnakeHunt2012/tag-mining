C = gcc
CXX = g++

MYHOME=../

INC = -I . -I $(MYHOME)/qmodule/qcommon-1.0.0/include -I $(MYHOME)/qmodule/transcode-1.0.0/include -I $(MYHOME)/qmodule/segment-2.2.1/include

OS = $(shell uname)
CFLAGS = -Wall 

CXXFLAGS_NOMDBM = $(CFLAGS)
CFLAGS += $(INC)
CXXFLAGS = $(CFLAGS)

RELEASE = 1

ifeq ($(RELEASE), 1)
	CXXFLAGS += -O3
else
	CXXFLAGS += -g
endif


OBJS = common.o 

BINDIR = ./

LIBS = $(MYHOME)/qmodule/segment-2.2.1/lib/libsegment.a $(MYHOME)/qmodule/transcode-1.0.0/lib/libtranscode.a

.PHONY: all

all: make-reverse-index

make-reverse-index: make-reverse-index.o $(OBJS)
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

clean:
	rm -f make-reverse-index *.o  *~ /cores/core.* core *.core *.a
