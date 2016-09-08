#!/bin/sh
LIBNAME=bcm2835

gcc -l$LIBNAME 2>&1 | grep -q "cannot find -l$LIBNAME"
if [ $? -eq 0 ] ; then
   printf "+--------------------------------------------------------------------------------+\n"
   printf "| \033[1;31mERROR: Library BCM2835 not found!\033[0m                                              |\n"
   printf "| Make sure you follow instructions from http://www.airspayce.com/mikem/bcm2835/ |\n"
   printf "| to install the BCM2835 library on your system first then install this addon.   |\n"
   printf "+--------------------------------------------------------------------------------+\n"
   exit 0
else
   echo "Library $LIBNAME found."
fi
