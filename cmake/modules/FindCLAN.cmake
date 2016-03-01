# Version 1.0
# Public Domain
# Written by Maxime SCHMITT <maxime.schmitt@etu.unistra.fr>

#/////////////////////////////////////////////////////////////////////////////#
#                                                                             #
# Search for the Chunky Loop Analyser Library on the system                   #
# Call with find_package(CLAN)                                                #
# The module defines:                                                         #
#   - CLAN_FOUND        - If CLAN was found                                   #
#   - CLAN_INCLUDE_DIRS - the CLAN include directories                        #
#   - CLAN_LIBRARIES    - the CLAN library directories                        #
#   - CLAN_EXECUTABLE   - the CLAN executable                                 #
#   - CLAN_VERSION      - the CLAN version                                    #
#                                                                             #
#/////////////////////////////////////////////////////////////////////////////#

include(LibFindMacros)

# Get hints about paths
libfind_pkg_check_modules(CLAN_PKGCONF clan)

# Headers
find_path(CLAN_INCLUDE_DIR
  NAMES "clan/clan.h"
  PATHS ${CLAN_PKGCONF_INCLUDE_DIRS})

# Version
libfind_version_header(CLAN "clan/macros.h" "CLAN_VERSION")

# Library
find_library(CLAN_LIBRARY
  NAMES clan
  PATHS ${CLAN_PKGCONF_LIBRARY_DIRS})

# Executable
find_program(CLAN_EXECUTABLE
  NAMES clan)

set(CLAN_PROCESS_LIBS CLAN_LIBRARY)
set(CLAN_PROCESS_INCLUDES CLAN_INCLUDE_DIR)
libfind_process(CLAN)
