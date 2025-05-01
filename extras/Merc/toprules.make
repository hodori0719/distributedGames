# -*- Mode:makefile; indent-tabs-mode:t -*-
#
# Contains variable declarations for the most part
# Feel free to override any of these variables in any of the SUBDIR files
TOPDIR = .

# class of release target: { debug | release | profile | pprof }
RELEASE = release

ARCH = $(shell arch)
OS = $(shell uname)

# compilers
CC=gcc
CPP=g++

# global includes if u want any
INCLUDES = 
DEFINES  = -D__$(ARCH)__ -D__$(OS)__
LIBRARIES  =
OPTFLAGS = -Wall -Wno-unused -Werror -fPIC
ifeq ($(RELEASE),debug)
 OPTFLAGS += -g -DBENCHMARK_REQUIRED -DREQUIRE_ASSERTIONS -DMERCURYID_PRINT_HEX # -DDEBUG 
else
ifeq ($(RELEASE),profile)
 OPTFLAGS += -pg -g
 LIBRARIES += -pg -g
else
ifeq ($(RELEASE),pprof)
 OPTFLAGS += -O2 -DNDEBUG
 LIBRARIES += -L$(HOME)/lib -L$(HOME)/local/lib -lprofiler
else
 OPTFLAGS += -O2 -DNDEBUG -DBENCHMARK_REQUIRED -DREQUIRE_ASSERTIONS -DMERCURYID_PRINT_HEX #-DWARN_ASSERTIONS
endif
endif
endif
ifeq ($(OS),Darwin)
 OPTFLAGS += -Wno-long-double
 # try to include fink libraries on Mac OS X
 DEFINES += -I/sw/include
 LIBRARIES += -L/sw/lib
endif
CFLAGS   = $(DEFINES) $(INCLUDES) $(OPTFLAGS)
CPPFLAGS = -include $(TOPDIR)/magic.h $(DEFINES) $(INCLUDES) $(OPTFLAGS) -Wno-deprecated -Wno-reorder
DEPFLAGS = -include $(TOPDIR)/magic.h $(INCLUDES) -w
LDFLAGS  = $(LIBRARIES)

ifeq  ($(OS),Linux)
LIBEXT := .so
endif
ifeq ($(OS),Darwin)
LIBEXT := .dylib
endif
ifeq ($(OS),Win32)
LIBEXT := .dll
endif

# nice little rules to do EVERYTHING! just make sure you name all 
# the code-sources as .cpp

srcs = $(wildcard *.cpp)
deps = $(patsubst %.cpp,.%.d,$(srcs))
objs = $(patsubst %.cpp,.%.o,$(srcs))

.%.o: %.cpp
	@echo "* Compiling $<"
	@$(CPP) -c $(CPPFLAGS) -o $@ $<

# make dependency files: the little 'sed' trick is required since
# we act more intelligent and prepend a 'dot' to the object files
.%.d: %.cpp
	@echo "+ Generating deps for $<"
	@$(CPP) $(DEPFLAGS) -MM $< | sed -e 's/^\([^ ]\)/\.\1/' > $@

# make deps if not cleaning
ifneq ($(deps),)
ifneq ($(MAKECMDGOALS),clean)
-include $(deps)
endif
endif
