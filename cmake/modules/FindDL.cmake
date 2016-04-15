# Version 1.0
# Public Domain
# Written by Maxime SCHMITT <maxime.schmitt@etu.unistra.fr>

#/////////////////////////////////////////////////////////////////////////////#
#                                                                             #
# Search for the Dynamic Linking loader library                               #
# Call with find_package(DL)                                                  #
# The module defines:                                                         #
#   - DL_FOUND        - If DL was found                                       #
#   - DL_INCLUDE_DIRS - the DL include directories                            #
#   - DL_LIBRARIES    - the DL library directories                            #
#                                                                             #
#/////////////////////////////////////////////////////////////////////////////#

include(LibFindMacros)

# Get hints about paths
libfind_pkg_check_modules(DL_PKGCONF dl)

# Headers
find_path(DL_INCLUDE_DIR
  NAMES "dlfcn.h"
  PATHS ${DL_PKGCONF_INCLUDE_DIRS})

# Library
find_library(DL_LIBRARY
  NAMES dl
  PATHS ${DL_PKGCONF_LIBRARY_DIRS})

set(DL_PROCESS_LIBS DL_LIBRARY)
set(DL_PROCESS_INCLUDES DL_INCLUDE_DIR)
libfind_process(DL)
