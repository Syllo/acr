# Version 1.0
# Public Domain
# Written by Maxime SCHMITT <maxime.schmitt@etu.unistra.fr>

#/////////////////////////////////////////////////////////////////////////////#
#                                                                             #
# Search for the Integer Set Library on the system                            #
# Call with find_package(ISL)                                                 #
# The module defines:                                                         #
#   - ISL_FOUND        - If ISL was found                                     #
#   - ISL_INCLUDE_DIRS - the ISL include directories                          #
#   - ISL_LIBRARIES    - the ISL library directories                          #
#   - ISL_VERSION      - the ISL version                                      #
#                                                                             #
#/////////////////////////////////////////////////////////////////////////////#

include(LibFindMacros)

# Get hints about paths
libfind_pkg_check_modules(ISL_PKGCONF isl)

# Headers
find_path(ISL_INCLUDE_DIR
  NAMES "isl/set.h"
  PATHS ${ISL_PKGCONF_INCLUDE_DIRS})

# Library
find_library(ISL_LIBRARY
  NAMES isl
  PATHS ${ISL_PKGCONF_LIBRARY_DIRS})

# Version
set(ISL_GET_VERSION_C
  "
  #include <stdlib.h>
  #include <stdio.h>
  #include <isl/version.h>

  int main(int argc, char **argv) {
    fprintf(stdout, \"%s\", isl_version())\;
    return EXIT_SUCCESS\;
  }
  ")

file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.c"
  ${ISL_GET_VERSION_C})

try_run(ISL_EXIT_CODE ISL_COMPILED
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.c
  COMPILE_DEFINITIONS "-DINCLUDE_DIRECTORIES:STRING=${ISL_INCLUDE_DIR}"
  LINK_LIBRARIES ${ISL_LIBRARY}
  RUN_OUTPUT_VARIABLE ISL_GET_VERSION_RUN_OUTPUT
  COMPILE_OUTPUT_VARIABLE ISL_COMPILE_RETURN)

if(ISL_COMPILED AND ISL_EXIT_CODE EQUAL 0)
  string(REGEX REPLACE ".*-([0-9]+[.][0-9]+[.][0-9]+)-.*" "\\1"
    ISL_VERSION_TEMP
    "${ISL_GET_VERSION_RUN_OUTPUT}")
  if(ISL_VERSION_TEMP STREQUAL "${ISL_GET_VERSION_RUN_OUTPUT}")
    string(REGEX REPLACE ".*-([0-9]+[.][0-9]+)-.*" "\\1"
      ISL_VERSION_TEMP
      "${ISL_GET_VERSION_RUN_OUTPUT}")
    if(ISL_VERSION_TEMP STREQUAL "${ISL_GET_VERSION_RUN_OUTPUT}")
      message(AUTHOR_WARNING "Unable to find ISL version in string: ${ISL_GET_VERSION_RUN_OUTPUT}")
    else()
      set(ISL_VERSION ${ISL_VERSION_TEMP})
    endif()
  else()
    set(ISL_VERSION ${ISL_VERSION_TEMP})
  endif()
else()
  message(AUTHOR_WARNING "Unable to run program to get ISL version")
endif()

set(ISL_PROCESS_LIBS ISL_LIBRARY)
set(ISL_PROCESS_INCLUDES ISL_INCLUDE_DIR)
libfind_process(ISL)
