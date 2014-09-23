# safeexec

Safely execute untrusted binaries from external source.

# Build

Clone this repository and run:

```
% cmake .
% make
```

# Install

You'll need root to install binary to set root setuid bit.

```
% sudo make install
```

# Test

```
% make
% sudo make permission
% make test
```

# History

This is copied from mooshak's SafeExec.

## Changes

* FreeBSD support
* Add some error handling
* Add chroot feature
