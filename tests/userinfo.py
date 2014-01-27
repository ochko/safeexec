import os
print("numbers on same line should be equal")
print("pwd:", os.getcwd()) #should work
print("uid:", os.getuid(), os.geteuid()) #should work
print("gid:", os.getgid(), os.getegid(), 1000, "should be same")

from inspect import currentframe
print("currrent filename", currentframe().f_code.co_filename)
