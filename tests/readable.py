import sys
if sys.version_info[0] == 3:
    filename = input()
else:
    filename = raw_input()

for line in open(filename):
    print(line[:-1]) # no extra newlines
