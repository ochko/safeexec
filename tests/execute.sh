#!/bin/sh

for input in *.c
do
  output=`echo -en $input | sed s/\.c$//`
  echo -en "Running $input...\n"
  gcc -o $output $input -Wall -ansi -pedantic
  ../safeexec --exec $output > /dev/null
  rm -f $output
  echo -en "\n"
done
