include ../toprules.make

TOPDIR = ..
INCLUDES += -I..
all: $(objs)

extraobjs = $(TARGET)

Application_MercRPC.h Application_MercRPC.cpp: ../mercury/Application.h
	./make_stub.pl -p Application -f mercury/Application.h -s GetPubsubStore -i Application $<

DIST_FILES = *.cpp *.h *.pl

include ../botrules.make
