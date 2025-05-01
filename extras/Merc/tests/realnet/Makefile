include ../../toprules.make
INCLUDES += -I$(TOPDIR)
LDFLAGS = -L$(TOPDIR)/  -L$(TNLDIR)
LIBS = -lcolyseus-wan -lpthread -lm 
merc_libs = $(TOPDIR)/libcolyseus-wan.so 

TOPDIR = ../..
TARGET = RealNetTest

all: $(TARGET)


$(TARGET): $(objs) $(merc_libs) 
	$(CPP) $(CFLAGS) $(LDFLAGS) $(objs) $(LIBS) -o $(TARGET) 

DIST_FILES = *.cpp *.h

include ../../botrules.make
