# Version 1.0
# Public Domain
# Written by Maxime SCHMITT <maxime.schmitt@etu.unistra.fr>

#/////////////////////////////////////////////////////////////////////////////#
#                                                                             #
# Search for the Chunky Loop Analyser Library on the system                   #
# Call with find_package(CLOOG)                                               #
# The module defines:                                                         #
#   - CLOOG_FOUND        - If CLOOG was found                                 #
#   - CLOOG_INCLUDE_DIRS - the CLOOG include directories                      #
#   - CLOOG_LIBRARIES    - the CLOOG library directories                      #
#   - CLOOG_EXECUTABLE   - the CLOOG executable                               #
#   - CLOOG_VERSION      - the CLOOG version                                  #
#                                                                             #
#/////////////////////////////////////////////////////////////////////////////#

include(LibFindMacros)

# Get hints about paths
libfind_pkg_check_modules(CLOOG_PKGCONF cloog)

# Headers
find_path(CLOOG_INCLUDE_DIR
  NAMES "cloog/cloog.h"
  PATHS ${CLOOG_PKGCONF_INCLUDE_DIRS})

# Version
if (NOT CLOOG_VERSION AND CLOOG_INCLUDE_DIR)
  set(filename "${CLOOG_INCLUDE_DIR}/cloog/version.h")
  if (NOT EXISTS ${filename})
    message(AUTHOR_WARNING "Unable to find ${filename}")
  else()
    file(READ "${filename}" header)
    set(cloog_major "CLOOG_VERSION_MAJOR")
    set(cloog_minor "CLOOG_VERSION_MINOR")
    set(cloog_patch "CLOOG_VERSION_REVISION")
    string(REGEX REPLACE ".*#[ \t]*define[ \t]*${cloog_major}[ \t]*([0-9]+).*"
      "\\1" match_major "${header}")
    string(REGEX REPLACE ".*#[ \t]*define[ \t]*${cloog_minor}[ \t]*([0-9]+).*"
      "\\1" match_minor "${header}")
    string(REGEX REPLACE ".*#[ \t]*define[ \t]*${cloog_patch}[ \t]*([0-9]+).*"
      "\\1" match_patch "${header}")
    if (match_major STREQUAL header OR
        match_minor STREQUAL header OR
        match_patch STREQUAL header)
      message(AUTHOR_WARNING "Unable to find CLOOG version from ${filename}")
    else()
      set(CLOOG_VERSION "${match_major}.${match_minor}.${match_patch}")
    endif()
  endif()
endif()

# Library
find_library(CLOOG_LIBRARY
  NAMES cloog-isl
  PATHS ${CLOOG_PKGCONF_LIBRARY_DIRS})

# Executable
find_program(CLOOG_EXECUTABLE
  NAMES cloog)

set(CLOOG_PROCESS_LIBS CLOOG_LIBRARY)
set(CLOOG_PROCESS_INCLUDES CLOOG_INCLUDE_DIR)
libfind_process(CLOOG)
