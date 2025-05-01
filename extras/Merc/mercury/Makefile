include ../toprules.make

SUBDIRS = 

TOPDIR = ..
INCLUDES += -I..
all: $(objs)

# this is used by scripts from the toplevel directory to 
# construct the distribution. path names should be relative
# to the current directory. 
# permitted: wildcards (globs) and dirnames 

DIST_FILES = *.cpp *.h

include ../botrules.make
