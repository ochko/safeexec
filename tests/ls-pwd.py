# does the unprivileged safeexec user have read
# access to the current directory?
# test with chmod o-r . or chmod o+r .

import os
cd=os.getcwd()
print(cd)
print(os.listdir(cd))
