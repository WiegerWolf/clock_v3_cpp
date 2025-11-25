set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

get_filename_component(MY_SYSROOT "${CMAKE_CURRENT_LIST_DIR}/pi_sysroot" ABSOLUTE)
set(SYSROOT_FLAGS "--sysroot=${MY_SYSROOT} -isystem ${MY_SYSROOT}/usr/include -isystem ${MY_SYSROOT}/usr/include/arm-linux-gnueabihf")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SYSROOT_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SYSROOT_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --sysroot=${MY_SYSROOT}")

# Tell CMake to look for programs on the Host, but Libs/Headers in the Sysroot
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
