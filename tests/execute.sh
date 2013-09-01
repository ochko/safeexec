#!/bin/sh

gcc -o  err    err.c     -Wall -ansi -pedantic
gcc -o  write  write.c   -Wall -ansi -pedantic 
gcc -o  read   read.c    -Wall -ansi -pedantic 
gcc -o  fork   fork.c    -Wall -ansi -pedantic 
gcc -o  io     io.c      -Wall -ansi -pedantic 
gcc -o  mle    mle.c     -Wall -ansi -pedantic 
gcc -o  ole    ole.c     -Wall -ansi -pedantic 
gcc -o  return return.c  -Wall -ansi -pedantic 
gcc -o  tle    tle.c     -Wall -ansi -pedantic 
gcc -o  wtle   wtle.c    -Wall -ansi -pedantic 

echo "*** Testing Memory Limit ***"
../safeexec --usage usage/memory-limit-exceeded.txt --exec mle > /dev/null
echo "------------------------------"

echo "*** Testing Output Limit ***"
../safeexec --usage usage/output-limit-exceeded.txt --exec ole    > /dev/null
echo "------------------------------"

echo "*** Testing Time Limit ***"
../safeexec --usage usage/timi-limit-exceeded.txt --exec tle    > /dev/null
echo "------------------------------"

echo "*** Testing Wall Time Limit ***"
../safeexec --usage usage/wall-time-limit-exceeded.txt --clock 3 --exec wtle   > /dev/null
echo "------------------------------"

echo "*** Testing Fork ***"
../safeexec --usage usage/can-not-fork.txt --exec fork
echo "------------------------------"

echo "*** Testing IO ***"
echo "100 Coder" | ../safeexec --usage usage/read-stdinput.txt --exec io
echo "------------------------------"

echo "*** Testing std err output ***"
../safeexec --usage usage/stderr-to-devnull.txt --exec err
echo "std err is redirected to /dev/null"
../safeexec --usage usage/stderr-to-file.txt --error err.out --exec err
echo "std err is redirected to err.out:"
cat err.out
rm -f err.out
echo "------------------------------"

echo "*** Testing file writing ***"
../safeexec --usage usage/file-write-attempt.txt --exec write
echo "------------------------------"

echo "*** Testing file reading ***"
../safeexec --usage usage/file-read-attempt.txt --error err.out --exec read
echo "------------------------------"
cat err.out
rm -f err.out
echo "------------------------------"

echo "*** Testing Exit code ***"
../safeexec --usage usage/nonzero-exit-code.txt --exec return > /dev/null
echo "------------------------------"

rm -f err read write fork io mle ole return tle wtle
