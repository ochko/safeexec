import sys
if sys.version_info[0] == 3:
    filename = input()
else:
    filename = raw_input()

f = open(filename, 'w')
f.write('This is a test\n')
f.close()
