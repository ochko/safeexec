# CMake generated Testfile for 
# Source directory: /home/ochko/safeexec
# Build directory: /home/ochko/safeexec
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
ADD_TEST(memory-limit-exceeded "tests/driver.rb" "test-mle" "--mem 32768" "Memory Limit Exceeded" "elapsed time: ? seconds" "memory usage: >32768 kbytes" "cpu usage: ? seconds")
ADD_TEST(time-limit-exceeded "tests/driver.rb" "test-tle" "--cpu 1" "Time Limit Exceeded" "elapsed time: >1 seconds" "memory usage: ? kbytes" "cpu usage: >1 seconds")
ADD_TEST(output-limit-exceeded "tests/driver.rb" "test-ole" " " "Time Limit Exceeded" "elapsed time: >1 seconds" "memory usage: ? kbytes" "cpu usage: >1 seconds")
ADD_TEST(fork-prevention "tests/driver.rb" "test-fork" "--nproc 0" "Command exited with non-zero status" "elapsed time: ? seconds" "memory usage: ? kbytes" "cpu usage: ? seconds")
ADD_TEST(wall-time-limit-exceeded "tests/driver.rb" "test-wtle" "--clock 3" "Time Limit Exceeded" "elapsed time: >3 seconds" "memory usage: ? kbytes" "cpu usage: ? seconds")
