include ../../toprules.make
INCLUDES += -I$(TOPDIR)
LDFLAGS += -L$(TOPDIR)
ifeq ($(RELEASE), profile)
merc_libs = $(TOPDIR)/libmerc-sim.a # $(TOPDIR)/libcolyseus-wan.a
LIBS = $(TOPDIR)/libmerc-sim.a -lpthread -lm -lz -lgmp -lgmpxx
else
LIBS = -lmerc-sim -lpthread -lm -lz -lgmp -lgmpxx
merc_libs = $(TOPDIR)/libmerc-sim.so
endif

extra_objs = 

TOPDIR = ../..
TARGET = simd

all: $(TARGET)

$(TARGET): $(objs) $(merc_libs) $(extra_objs)
	$(CPP) $(LDFLAGS) $(objs) $(extra_objs) $(LIBS) -o $(TARGET)

DIST_FILES = *.cpp *.h *.cxx *.cfg

include ../../botrules.make

