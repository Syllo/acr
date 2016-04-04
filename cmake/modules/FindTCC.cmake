# Version 1.0
# Public Domain
# Written by Maxime SCHMITT <maxime.schmitt@etu.unistra.fr>

#/////////////////////////////////////////////////////////////////////////////#
#                                                                             #
# Search for the OpenScop Library on the system                               #
# Call with find_package(TCC)                                                 #
# The module defines:                                                         #
#   - TCC_FOUND        - If TCC was found                                     #
#   - TCC_INCLUDE_DIRS - the TCC include directories                          #
#   - TCC_LIBRARIES    - the TCC library directories                          #
#   - TCC_EXECUTABLE   - the TCC executable                                   #
#   - TCC_VERSION      - the TCC version                                      #
#                                                                             #
#/////////////////////////////////////////////////////////////////////////////#

include(LibFindMacros)

# Get hints about paths
libfind_pkg_check_modules(TCC_PKGCONF tcc)

# Headers
find_path(TCC_INCLUDE_DIR
  NAMES "libtcc.h"
  PATHS ${TCC_PKGCONF_INCLUDE_DIRS})

# Library
find_library(TCC_LIBRARY
  NAMES tcc
  PATHS ${TCC_PKGCONF_LIBRARY_DIRS})

# Executable
find_program(TCC_EXECUTABLE
  NAMES tcc)

# Version
if(TCC_EXECUTABLE)
  exec_program(${TCC_EXECUTABLE}
    ARGS "-v"
    OUTPUT_VARIABLE TCC_VERSION_EXEC_OUTPUT
    RETURN_VALUE TCC_VERSION_EXEC_RETVAL)
  if(TCC_VERSION_EXEC_RETVAL EQUAL 0)
    string(REGEX REPLACE ".*[ \t]*([0-9]+.[0-9]+.[0-9]+).*" "\\1"
      TCC_VERSION_TEMP ${TCC_VERSION_EXEC_OUTPUT})
    if(TCC_VERSION_TEMP STREQUAL TCC_VERSION_EXEC_OUTPUT)
      message(AUTHOR_WARNING "Unable to get TCC version from string")
    else()
      set(TCC_VERSION ${TCC_VERSION_TEMP})
    endif()
  endif()
endif()


set(TCC_PROCESS_LIBS TCC_LIBRARY)
set(TCC_PROCESS_INCLUDES TCC_INCLUDE_DIR)
libfind_process(TCC)
