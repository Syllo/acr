option(CLAN_BUNDLED "Download and use CLAN internally")
if(CLAN_BUNDLED OR ALL_DEP_BUNDLED)
  ExternalProject_Add(clan_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/clan"
    GIT_REPOSITORY "https://github.com/Syllo/clan"
    GIT_TAG dev
    UPDATE_COMMAND ""
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${DEP_INSTALL_DIR}"
               "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
               "-DCMAKE_SKIP_RPATH=TRUE"
               "-DCMAKE_BUILD_TYPE=Release")
  if(TARGET osl_external)
    add_dependencies(clan_external osl_external)
  endif()
  add_library(clan INTERFACE IMPORTED)
  add_dependencies(clan clan_external)
  set_property(TARGET clan PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${DEP_INCLUDE_DIR})
  set_property(TARGET clan PROPERTY INTERFACE_LINK_LIBRARIES
    ${DEP_LIB_DIR}/libclan.so)
else(CLAN_BUNDLED OR ALL_DEP_BUNDLED)
  if(CLAN_FOUND)
    message(WARNING "CLAN version ${CLAN_VERSION} is too old")
  endif()
  message(FATAL_ERROR "CLAN not found but Required"
    ". If you want ACR to download and use it add -DCLAN_BUNDLED=YES to CMake"
    " command line")
endif(CLAN_BUNDLED OR ALL_DEP_BUNDLED)
