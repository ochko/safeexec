# safeexec

Safely execute untrusted binaries from external source.

# Build

Clone this repository and run

```
% cmake . && make
```

# Install

You'll need root to install binary to set root setuid bit.

```
% chown root safeexed
% chmod u+s safeexed
```

# Test

Test are in `./tests` directory.

```
% cd tests
% ./execute.sh
```

# History

This is safe executor extends mooshak's SafeExec by adding chroot feature.
