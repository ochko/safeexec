# safeexec

Safely execute untrusted binaries from external source.

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

```
% make
% sudo make permission
% make test
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
