option(TCC_BUNDLED "Download and use TCC internaly")
if(TCC_BUNDLED OR ALL_DEP_BUNDLED)
  ExternalProject_Add(tcc_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/tcc"
    URL http://download.savannah.gnu.org/releases/tinycc/tcc-0.9.26.tar.bz2
    URL_HASH
    SHA256=521e701ae436c302545c3f973a9c9b7e2694769c71d9be10f70a2460705b6d71
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND
    "./configure"
    "--cc=${CMAKE_C_COMPILER}"
    "--disable-static"
    "--extra-cflags=-O3 -g ${COMPILER_LTO_FLAG}"
    "--extra-ldlags=-O3 -g ${COMPILER_LTO_FLAG}"
    "--prefix=${DEP_INSTALL_DIR}"
    UPDATE_COMMAND ""
    BUILD_COMMAND
    "make"
    INSTALL_COMMAND
    "make"
    "install")
  add_library(tcc INTERFACE IMPORTED)
  add_dependencies(tcc tcc_external)
  set_property(TARGET tcc PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${DEP_INCLUDE_DIR})
  set_property(TARGET tcc PROPERTY INTERFACE_LINK_LIBRARIES
    ${DEP_LIB_DIR}/libtcc.so)
else(TCC_BUNDLED OR ALL_DEP_BUNDLED)
  message(WARNING "Tiny C Compiler not found, this may lower ACR performance"
    ". If you want ACR to download and use it add -DTCC_BUNDLED=YES to CMake"
    " command line")
endif(TCC_BUNDLED OR ALL_DEP_BUNDLED)
