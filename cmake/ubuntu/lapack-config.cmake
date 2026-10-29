# Stand-in for the CMake config of reference LAPACK, which Ubuntu's
# liblapack-dev does not ship. kcm-howdy only checks that LAPACK is present
# and links plain "-llapack", which dlib already pulls in.
if(NOT EXISTS "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/liblapack.so")
    set(lapack_FOUND FALSE)
    set(lapack_NOT_FOUND_MESSAGE "liblapack.so not found, install liblapack-dev")
    return()
endif()
set(lapack_FOUND TRUE)
