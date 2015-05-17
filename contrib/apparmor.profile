#include <tunables/global>

/path/to/SafeExec/safeexec {
  #include <abstractions/base>

  capability dac_override,
  capability fowner,
  capability setuid,

  /opt/ruby/bin/* wcx -> ruby,
  /opt/python/bin/* wcx -> python,
  /path/to/SafeExec/solutions/*/exe/* wcx -> solution,
  /proc/*/status r,

  profile ruby {
    /path/to/SafeExec/sandbox/* r,
    /etc/ld.so.cache r,
    /lib/lib*so* mr,
    /dev/urandom r,
    /etc/locale.alias r,
    /usr/lib/locale/* r,
    /opt/ruby/lib/ruby/**/*.so mr,
  }

  profile python {
    /path/to/SafeExec/sandbox/* r,
    /etc/ld.so.cache r,
    /lib/lib*so* mr,
    /proc/meminfo r,
    /opt/python/lib/python3.2/*.py r,
    /opt/python/lib/python3.2/*.pyc r,
    /opt/python/lib/python3.2/**/*.py r,
    /opt/python/lib/python3.2/**/*.pyc r,
    /opt/python/lib/python3.2/**/*.so mr,
    /opt/python/include/python3.2/*.h r,
    /opt/python/include/python3.2m/*.h r,
    /opt/python/lib/python3.2/site-packages/ r,
    /opt/python/lib/python3.2/config-3.2m/Makefile r,
  }

  profile solution {
    /etc/ld.so.cache r,
    /etc/locale.alias r,
    /etc/nsswitch.conf r,
    /etc/passwd r,
    /lib/lib*so* mr,
    /lib/libc-*.so r,
    /lib/libm-*.so r,
    /usr/lib/lib*so.* mr,
    /lib/tls/i686/cmov/libc-*.so mr,
    /lib/tls/i686/cmov/libdl-*.so mr,
    /lib/tls/i686/cmov/libm-*.so mr,
    /lib/tls/i686/cmov/libnsl-*.so mr,
    /lib/tls/i686/cmov/libnss_compat-*.so mr,
    /lib/tls/i686/cmov/libnss_files-*.so mr,
    /lib/tls/i686/cmov/libnss_nis-*.so mr,
    /lib/tls/i686/cmov/libpthread-*.so mr,
    /lib/tls/i686/cmov/librt-*.so mr,
    /proc/*/maps r,
    /proc/*/stat r,
    /proc/cpuinfo r,
    /proc/stat r,
    /usr/lib/gconv/* r,
    /dev/tty rw,
  }
}
