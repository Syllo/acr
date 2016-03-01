# Version 1.0
# Public Domain
# Written by Maxime SCHMITT <maxime.schmitt@etu.unistra.fr>

#/////////////////////////////////////////////////////////////////////////////#
#                                                                             #
# Search for the OpenScop Library on the system                               #
# Call with find_package(OSL)                                                 #
# The module defines:                                                         #
#   - OSL_FOUND        - If OSL was found                                     #
#   - OSL_INCLUDE_DIRS - the OSL include directories                          #
#   - OSL_LIBRARIES    - the OSL library directories                          #
#   - OSL_VERSION      - the OSL version                                      #
#                                                                             #
#/////////////////////////////////////////////////////////////////////////////#

include(LibFindMacros)

# Get hints about paths
libfind_pkg_check_modules(OSL_PKGCONF osl)

# Headers
find_path(OSL_INCLUDE_DIR
  NAMES "osl/osl.h"
  PATHS ${OSL_PKGCONF_INCLUDE_DIRS})

# Version
libfind_version_header(OSL "osl/scop.h" "OSL_RELEASE")

# Library
find_library(OSL_LIBRARY
  NAMES osl
  PATHS ${OSL_PKGCONF_LIBRARY_DIRS})

set(OSL_PROCESS_LIBS OSL_LIBRARY)
set(OSL_PROCESS_INCLUDES OSL_INCLUDE_DIR)
libfind_process(OSL)
