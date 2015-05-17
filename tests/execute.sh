#!/bin/sh

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
