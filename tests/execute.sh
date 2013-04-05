#!/bin/sh

gcc -o  err    err.c     -Wall -ansi -pedantic
gcc -o  file   file.c    -Wall -ansi -pedantic 
gcc -o  fork   fork.c    -Wall -ansi -pedantic 
gcc -o  io     io.c      -Wall -ansi -pedantic 
gcc -o  mle    mle.c     -Wall -ansi -pedantic 
gcc -o  ole    ole.c     -Wall -ansi -pedantic 
gcc -o  return return.c  -Wall -ansi -pedantic 
gcc -o  tle    tle.c     -Wall -ansi -pedantic 
gcc -o  wtle   wtle.c    -Wall -ansi -pedantic 

echo "*** Testing Memory Limit ***"
../safeexec --exec mle > /dev/null
echo "------------------------------"

echo "*** Testing Output Limit ***"
../safeexec --exec ole    > /dev/null
echo "------------------------------"

echo "*** Testing Time Limit ***"
../safeexec --exec tle    > /dev/null
echo "------------------------------"

echo "*** Testing Wall Time Limit ***"
../safeexec --exec wtle   > /dev/null
echo "------------------------------"

echo "*** Testing Fork ***"
../safeexec --exec fork
echo "------------------------------"

echo "*** Testing IO(setuid) ***"
echo "100 Coder" | ../safeexec --exec io
echo "------------------------------"

echo "*** Testing std err output ***"
../safeexec --exec err
echo "  std err is redirected to /dev/null"
../safeexec --error err.out --exec err
echo "  std err is redirected to err.out"
cat err.out
rm -f err.out
echo "------------------------------"

echo "*** Testing file writing ***"
../safeexec --exec file
echo "------------------------------"

echo "*** Testing Exit code ***"
../safeexec --exec return > /dev/null
echo "------------------------------"

rm -f err file fork io mle ole return tle wtle
