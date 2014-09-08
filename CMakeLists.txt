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

add_executable(safeexec safeexec.c error.c safe.c platform/${PLATFORM}/memusage.c)

target_link_libraries(safeexec ${LINKED_LIBS})
