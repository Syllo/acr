option(CLOOG_BUNDLED "Download and use CLOOG internaly")
if(CLOOG_BUNDLED OR ALL_DEP_BUNDLED)
  if(TARGET osl_external)
    set(CLOOG_OSL_PREFIX "--with-osl-prefix=${DEP_INSTALL_DIR}")
  endif()
  if(TARGET isl_external)
    set(CLOOG_ISL_PREFIX "--with-isl-prefix=${DEP_INSTALL_DIR}")
  endif()
  ExternalProject_Add(cloog_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/cloog"
    GIT_REPOSITORY https://github.com/periscop/cloog.git
    GIT_TAG cloog-0.18.4
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND
    "./autogen.sh"
    "&&"
    "CC=${CMAKE_C_COMPILER}"
    "./configure"
    "--with-isl=system"
    "${CLOOG_ISL_PREFIX}"
    "--with-osl=system"
    "${CLOOG_OSL_PREFIX}"
    "--with-gmp=system"
    "--prefix=${DEP_INSTALL_DIR}"
    UPDATE_COMMAND ""
    BUILD_COMMAND
    "make"
    INSTALL_COMMAND
    "make"
    "install")
  if(TARGET osl_external)
    add_dependencies(cloog_external osl_external)
  endif()
  if(TARGET isl_external)
    add_dependencies(cloog_external isl_external)
  endif()
  add_library(cloog INTERFACE IMPORTED)
  add_dependencies(cloog cloog_external)
  set_property(TARGET cloog PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${DEP_INCLUDE_DIR})
  set_property(TARGET cloog PROPERTY INTERFACE_LINK_LIBRARIES
    ${DEP_LIB_DIR}/libcloog-isl.so)
  set_property(TARGET cloog PROPERTY INTERFACE_COMPILE_DEFINITIONS
    CLOOG_INT_GMP)
else(CLOOG_BUNDLED OR ALL_DEP_BUNDLED)
  message(FATAL_ERROR
    "Chunky Loop Generator (CLooG) not found but required"
    ". If you want ACR to download and use it add -DCLOOG_BUNDLED=YES to CMake"
    " command line")
endif(CLOOG_BUNDLED OR ALL_DEP_BUNDLED)
