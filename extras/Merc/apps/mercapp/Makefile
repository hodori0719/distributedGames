include ../../toprules.make
INCLUDES += -I$(TOPDIR)
LDFLAGS += -L$(TOPDIR)
ifeq ($(RELEASE),profile)
merc_libs = $(TOPDIR)/libmerc-wan.a
LIBS = $(merc_libs) -lpthread -lm -lz -lgmp
else
LIBS = -lmerc-wan -lpthread -lm -lz -lgmp 
merc_libs = $(TOPDIR)/libmerc-wan.so
endif

TOPDIR = ../..
TARGET = mercapp

all: $(TARGET) copy

.PHONY: copy
$(TARGET): $(objs) $(merc_libs) 
	$(CPP) $(CFLAGS) $(LDFLAGS) $(objs) $(LIBS) -o $(TARGET)

copy: $(TARGET)
	@echo "+ Moving $(TARGET) to build/"
	@mkdir -p $(TOPDIR)/build
	@cp $(TARGET) $(TOPDIR)/build

DIST_FILES = *.cpp *.h *.cfg

include ../../botrules.make
