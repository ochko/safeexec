cmake_minimum_required(VERSION 2.8)

project(safeexec)

include_directories(${CMAKE_SOURCE_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(PLATFORM bsd)
    set(LINKED_LIBS kvm)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(PLATFORM linux)
else ()
    message(FATAL_ERROR "This OS is not supported.")
endif ()

add_executable(safeexec safeexec.c error.c safe.c setlimit.c platform/${PLATFORM}/setlimits.c platform/${PLATFORM}/memusage.c)

target_link_libraries(safeexec ${LINKED_LIBS})

add_custom_target(permission chown root safeexec
                  COMMAND chmod u+s safeexec)

add_executable(tests/test-stderr tests/test-stderr.c)
add_executable(tests/test-write  tests/test-write.c )
add_executable(tests/test-read   tests/test-read.c  )
add_executable(tests/test-fork   tests/test-fork.c  )
add_executable(tests/test-io     tests/test-io.c    )
add_executable(tests/test-mle    tests/test-mle.c   )
add_executable(tests/test-ole    tests/test-ole.c   )
add_executable(tests/test-ret    tests/test-ret.c   )
add_executable(tests/test-tle    tests/test-tle.c   )
add_executable(tests/test-wtle   tests/test-wtle.c  )

enable_testing()

add_test(memory-limit-exceeded tests/driver.rb test-mle
         "--mem 32768"
         "Memory Limit Exceeded"
         "elapsed time: ? seconds"
         "memory usage: >32768 kbytes"
         "cpu usage: ? seconds")

add_test(time-limit-exceeded tests/driver.rb test-tle
         "--cpu 1"
         "Time Limit Exceeded"
         "elapsed time: >1 seconds"
         "memory usage: ? kbytes"
         "cpu usage: >1 seconds")

add_test(output-limit-exceeded tests/driver.rb test-ole
         " "
         "Time Limit Exceeded"
         "elapsed time: >1 seconds"
         "memory usage: ? kbytes"
         "cpu usage: >1 seconds")

add_test(fork-attemp tests/driver.rb test-fork
         "--nproc 0"
         "Command exited with non-zero status"
         "elapsed time: ? seconds"
         "memory usage: ? kbytes"
         "cpu usage: ? seconds")
         
add_test(file-write-prepare chmod 0644 tests/file.dat)
add_test(file-write-attemp tests/driver.rb test-write
         " "
         "Command exited with non-zero status"
         "elapsed time: ? seconds"
         "memory usage: ? kbytes"
         "cpu usage: ? seconds")

add_test(file-read-prepare chmod 0620 tests/secret.txt)
add_test(file-read-attemp tests/driver.rb test-read
         " "
         "Command exited with non-zero status"
         "elapsed time: ? seconds"
         "memory usage: ? kbytes"
         "cpu usage: ? seconds")

add_test(return-code tests/driver.rb test-ret
         " "
         "Command exited with non-zero status"
         "elapsed time: ? seconds"
         "memory usage: ? kbytes"
         "cpu usage: ? seconds")

add_test(wall-time-limit-exceeded tests/driver.rb test-wtle
         "--clock 3"
         "Time Limit Exceeded"
         "elapsed time: >3 seconds"
         "memory usage: ? kbytes"
         "cpu usage: ? seconds")

install(TARGETS safeexec
        DESTINATION "bin"
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(SCRIPT chown.cmake)
