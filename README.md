# safeexec(safe execution environment)

A general-purpose lightweight sandbox for safely executing user programs.
safeexec provides a sandbox environment which you can set limits on system resources like memory, cpu time effectively preventing intended or unintented breach of system.
However, running untrusted code on your server should be taken at *your own risk*.

## How it works

- Safeexec forks child process.
- Sets various limitations on the child process via `setrlimit`. So safeexec should have setuid bit on, and uid root.
- Parent process polls memory usage looking into `/proc` filesystem on linux or via `kvm` on freebsd.
- Wall clock limit is enforced with alarm() & signal().
- Reports usage stats to `stdout` or file specified by `--usage` argument.

### Resource usage report format

Resource usage is in 4 line plain text.

For memory limit:
```
Memory Limit Exceeded
elapsed time: 0 seconds
memory usage: 32884 kbytes
cpu usage: 0.416 seconds
```

For time limit:
```
Time Limit Exceeded
elapsed time: 2 seconds
memory usage: 1424 kbytes
cpu usage: 1.000 seconds
```

File write attempt:
```
Command exited with non-zero status (1)
elapsed time: 0 seconds
memory usage: 64 kbytes
cpu usage: 0.000 seconds
```

## Usage notes

Running it with no argument gives:

```
usage: ./safeexec <options> --exec <command>
Available options:
	--cpu     <seconds>           Default: 1 second(s)
	--mem     <kbytes>            Default: 32768 kbyte(s)
	--space   <kbytes>            Default: 0 kbyte(s)
	--uids    <minuid> <maxuid>   Default: 5000-65535
	--minuid  <uid>               Default: 5000
	--maxuid  <uid>               Default: 65535
	--core    <kbytes>            Default: 0 kbyte(s)
	--nproc   <number>            Default: 0 proccess(es)
	--fsize   <kbytes>            Default: 8192 kbyte(s)
	--stack   <kbytes>            Default: 8192 kbyte(s)
	--clock   <seconds>           Wall clock timeout (default: 10)
	--usage   <filename>          Report statistics to ... (default: stderr)
	--chroot  <path>              Directory to chrooted (default: /tmp)
	--error   <path>              Print stderr to file (default: /dev/null)
```

Keep in mind that `--exec` must be last option.

### Memory limits

For compiled languages like C or C++ you can set strict memory limitation.
But for interpreted languages(python, ruby, java etc..) you can't set strict memory limits. Instead you have to give big enough memory that your language can load.

### Unpriviliged uid range

Safeexec runs users program as a random uid. Default gid/uid range is: 5000-65535.
So please check if any uid in this range is in use. If then you can chage the range with arguments. `--minuid` and `--maxuid`.
Whole user range is treated as "unpriviliged" ("other" for all files).

### Network access

On Freebsd network access blocked by setting limit on socket buffer size.
But on Linux it doesn't block network access. Use other tools(iptables,..) for network access blockage.

### Number of open files

Maximum number of open files is not limited because vm(java) or interpreted(ruby, python) languages open a lots of files.

### Chroot

Chroot is powerful stuff: it makes a given directory (say, /jail)
appear to be the root directory of the file system, as far as
the child process can see. However, you need to install needed libraries into jail to run user programs properly.
For example, a Python3 jail needs to include some unexpected things like /lib/libcrypt-2.5.so.
You can use safeexec without chroot.

## Build

Clone this repository and run:

```
% cmake .
% make
```

## Install

You'll need root to install binary to set root setuid bit.

```
% sudo make install
```

## Test

`% sudo make permission && make test`

```
Running tests...
Test project /usr/home/ochko/safeexec
    Start 1: memory-limit-exceeded
1/8 Test #1: memory-limit-exceeded ............   Passed    1.16 sec
    Start 2: time-limit-exceeded
2/8 Test #2: time-limit-exceeded ..............   Passed    2.10 sec
    Start 3: output-limit-exceeded
3/8 Test #3: output-limit-exceeded ............   Passed    2.10 sec
    Start 4: fork-attemp
4/8 Test #4: fork-attemp ......................   Passed    0.59 sec
    Start 5: file-write-attemp
5/8 Test #5: file-write-attemp ................   Passed    0.13 sec
    Start 6: file-read-attemp
6/8 Test #6: file-read-attemp .................   Passed    0.13 sec
    Start 7: return-code
7/8 Test #7: return-code ......................   Passed    0.13 sec
    Start 8: wall-time-limit-exceeded
8/8 Test #8: wall-time-limit-exceeded .........   Passed    3.13 sec

100% tests passed, 0 tests failed out of 8

Total Test time (real) =   9.48 sec
```

## Todo

* Fix file read/write permissions on linux
* Use [Getopt](http://www.gnu.org/software/libc/manual/html_node/Getopt.html) similar argument parsing library.

## Changelog

* FreeBSD support

### May 17, 2015.

* Fix linux specifics

### Sep 23, 2014.

* chmod/chown task for cmake
* Add more test

### Sep 8, 2014.

* Introduce cmake
* Terminate only process under controll. kill(pid,..) instead of kill(-1,...)

### Apr 5-6, 2013

* Apparmor profile for linux file read/write permission workaround.
* setgit to protect files with group permission
* Redirect stderr of user program

### Dec 10, 2012.

* Add address space limit
* Error handling for permission setter

### Dec 8, 2012.

* FreeBSD support using `kvm.h`
* Add some error handling

### Feb 16, 2009.

* Copied from mooshak's SafeExec.
* Add chroot feature

## License

The MIT License (MIT)

Copyright (c) Lkhagva Ochirkhuyag, 2009-2015

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
