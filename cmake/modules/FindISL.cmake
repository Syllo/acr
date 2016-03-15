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

set(ISL_PROCESS_LIBS ISL_LIBRARY)
set(ISL_PROCESS_INCLUDES ISL_INCLUDE_DIR)
libfind_process(ISL)
