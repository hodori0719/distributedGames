include ../../toprules.make
INCLUDES += -I$(TOPDIR)
LDFLAGS  += -L$(TOPDIR) 
LIBS     += -lmerc-wan -lpthread -lm -lreadline -lncurses -lz -lgmp

TOPDIR = ../..

all: client server 

client: .TimeTestClient.o
	@echo "+ Linking $@"
	@$(CPP) $(CFLAGS) $(LDFLAGS) .TimeTestClient.o $(LIBS) -o $@

server: .TimeTestServer.o
	@echo "+ Linking $@"
	@$(CPP) $(CFLAGS) $(LDFLAGS) .TimeTestServer.o $(LIBS) -o $@

DIST_FILES = *.cpp *.h *.pl

include ../../botrules.make
