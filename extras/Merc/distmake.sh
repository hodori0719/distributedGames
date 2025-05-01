#!/bin/sh
#
# Run distributed make to parallelize compilation
#
# Install: http://distcc.samba.org/
# dont put DISTCC_HOSTS in here since they could be 
# different for me and you!

# DISTCC_HOSTS='localhost iris-d-07 iris-d-10'
# export DISTCC_HOSTS

make -j 6 CPP='distcc g++32' CC='distcc gcc32' $*
