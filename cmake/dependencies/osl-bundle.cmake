option(OSL_BUNDLED "Download and use CLAN internally")
if(OSL_BUNDLED OR ALL_DEP_BUNDLED)
  ExternalProject_Add(osl_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/osl"
    GIT_REPOSITORY "https://github.com/periscop/openscop"
    UPDATE_COMMAND ""
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${DEP_INSTALL_DIR}"
               "-DCMAKE_LIBRARY_PATH=${DEP_INSTALL_DIR}"
               "-DCMAKE_SKIP_RPATH=TRUE")
  add_library(osl INTERFACE IMPORTED)
  add_dependencies(osl osl_external)
  set_property(TARGET osl PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${DEP_INCLUDE_DIR})
  set_property(TARGET osl PROPERTY INTERFACE_LINK_LIBRARIES
    ${DEP_LIB_DIR}/libosl.so)
else(OSL_BUNDLED OR ALL_DEP_BUNDLED)
  message(FATAL_ERROR "OSL not found but Required"
    ". If you want ACR to download and use it add -DOSL_BUNDLED=YES"
    " to CMake command line")
endif(OSL_BUNDLED OR ALL_DEP_BUNDLED)
