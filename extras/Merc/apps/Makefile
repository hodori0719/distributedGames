include ../toprules.make

# XXX ick. for distribution testing, SUBDIRS has to be a 
# subset of DIST_FILES 
SUBDIRS = bootstrap sim-driver mercapp chkjoin2 # testapp #microbench

all install clean: $(SUBDIRS)

DIST_FILES = bootstrap sim-driver mercapp chkjoin2 

include ../botrules.make
