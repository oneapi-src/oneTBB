include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers/GNU.cmake)

# Remove dl library not present in QNX systems
unset(TBB_COMMON_LINK_LIBS)