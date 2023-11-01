add_library(previewproducer STATIC
    previewproducer.cpp
    capturethread.cpp
    )

target_link_libraries(previewproducer PRIVATE
    Qt5::Core
    Qt5::Gui
    KF5::I18n
    dlib::dlib
    lapack
    ${OpenCV_LIBS}
    ${OpenBLAS_LIBRARIES}
)

# enable AVX instructions for x86, dlib is 30% faster
set(gcc_like_compilers GNU Clang  Intel)
set(intel_archs x86_64 i386 i686 AMD64 amd64 x86)
if ((";${gcc_like_compilers};" MATCHES ";${CMAKE_CXX_COMPILER_ID};")  AND
    (";${intel_archs};"        MATCHES ";${CMAKE_SYSTEM_PROCESSOR};") AND USE_AUTO_VECTOR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
endif()

# opencv uses exceptions
kde_target_enable_exceptions(previewproducer PRIVATE)
