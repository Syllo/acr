option(ISL_BUNDLED "Download and use ISL internaly")
if(ISL_BUNDLED OR ALL_DEP_BUNDLED)
  ExternalProject_Add(isl_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/isl"
    GIT_REPOSITORY git://repo.or.cz/isl.git
    GIT_TAG isl-0.16.1
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND
    "./autogen.sh"
    "&&"
    "CC=${CMAKE_C_COMPILER}"
    "./configure"
    "--with-int=gmp"
    "--with-gmp=system"
    "--prefix=${DEP_INSTALL_DIR}"
    UPDATE_COMMAND ""
    BUILD_COMMAND
    "make"
    INSTALL_COMMAND
    "make"
    "install")
  add_library(isl INTERFACE IMPORTED)
  add_dependencies(isl isl_external)
  set_property(TARGET isl PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${DEP_INCLUDE_DIR})
  set_property(TARGET isl PROPERTY INTERFACE_LINK_LIBRARIES
    ${DEP_LIB_DIR}/libisl.so)
else(ISL_BUNDLED OR ALL_DEP_BUNDLED)
  if(ISL_FOUND)
    message(FATAL_ERROR "ISL version >= 0.16 required but found ${ISL_VERSION}")
  else()
    message(FATAL_ERROR
      "Integer Set Library (ISL) not found but required"
      ". If you want ACR to download and use it add -DISL_BUNDLED=YES to CMake"
      " command line")
  endif()
endif(ISL_BUNDLED OR ALL_DEP_BUNDLED)
