include ../../toprules.make

INCLUDES += -I$(TOPDIR)
LDFLAGS = -L$(TOPDIR)
LIBS =  -lmerc-wan -lpthread -lm -lz -lgmp 
merc_libs = $(TOPDIR)/libmerc-wan.so

TOPDIR = ../..
TARGET = bootstrap

all: $(TARGET) copy

.PHONY: copy

$(TARGET): $(objs) $(merc_libs)
	@echo "+ Linking $@"
	$(CPP) $(CFLAGS) $(LDFLAGS) $(objs) $(LIBS) -o $@

copy: $(TARGET)
	@echo "+ Moving $(TARGET) to build/"
	@mkdir -p $(TOPDIR)/build
	@cp $(TARGET) $(TOPDIR)/build

DIST_FILES = *.cpp *.h

include ../../botrules.make
