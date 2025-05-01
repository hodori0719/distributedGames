include ../../toprules.make
INCLUDES += -I$(TOPDIR)
LDFLAGS = -L$(TOPDIR)
LIBS = -lcolyseus-wan -lpthread -lm -lreadline -lncurses -lz -lgmp
merc_libs = $(TOPDIR)/libcolyseus-wan.so

TOPDIR = ../..
TARGET = merctest
SCHEMA = om_test.cfg

all: $(TARGET)

.PHONY: copyit

$(TARGET): $(objs) $(merc_libs) 
	@echo "+ Linking $@"
	@$(CPP) $(CFLAGS) $(LDFLAGS) $(objs) $(LIBS) -o $(TARGET)

DIST_FILES = *.cpp *.h *.pl

include ../../botrules.make
