# only works in Python 3

# demonstrates that the user can change the thread name
# which is probably not a big deal

# run "ps Hcx -ef" to see the effect

def set_proc_name(newname):
    from ctypes import cdll, byref, create_string_buffer
    libc = cdll.LoadLibrary('libc.so.6')
    buff = create_string_buffer(len(newname)+1)
    buff.value = newname
    libc.prctl(15, byref(buff), 0, 0, 0)
    
def get_proc_name():
    from ctypes import cdll, byref, create_string_buffer
    libc = cdll.LoadLibrary('libc.so.6')
    buff = create_string_buffer(128)
    # 16 == PR_GET_NAME from <linux/prctl.h>
    libc.prctl(16, byref(buff), 0, 0, 0)
    return buff.value

import sys
# sys.argv[0] == 'python'

# outputs 'python'
print(get_proc_name())

set_proc_name(bytes('this is the new process name', 'utf-8'))

# outputs 'this is'...
print(get_proc_name())
