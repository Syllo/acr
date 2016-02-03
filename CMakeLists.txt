cmake_minimum_required(VERSION 2.8)

#///////////////////////////////////////////////////////////////////#
#                              PROJECT                              #
#///////////////////////////////////////////////////////////////////#

set(ACR_VERSION_MAJOR 0)
set(ACR_VERSION_MINOR 0)
set(ACR_VERSION_PATCH 0)
if(NOT PACKAGE_VERSION)
  set(PACKAGE_VERSION
    ${ACR_VERSION_MAJOR}.${ACR_VERSION_MINOR}.${ACR_VERSION_PATCH})
endif()

project(acr
  LANGUAGES C CXX)

# Default to Debug build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug) # Set "Release" otherwise
  message(STATUS "No building type advertised, default to Debug")
endif()

#///////////////////////////////////////////////////////////////////#
#                           DEPENDENCIES                            #
#///////////////////////////////////////////////////////////////////#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

find_package(CLAN )#QUIET)
find_package(OSL )#QUIET)
if(OSL_FOUND AND CLAN_FOUND)
  message (STATUS "CLAN and OSL are installed")
else()
  message (WARNING "CLAN and OSL not installed\n"
    "Fallback by trying to clone the repository from source")
  find_package(Git REQUIRED)
  execute_process(COMMAND
    ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE OSL_SUBMODULE_UPDATE)
  if(NOT OSL_SUBMODULE_UPDATE EQUAL 0)
    message(FATAL_ERROR "Unable to clone CLAN, dependency not met")
  else()
    message(STATUS "CLAN successfully cloned, configuring")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/clan)
    set(USING_CLAN_GIT TRUE)
  endif()
else()
  message(STATUS "Found lib osl")
  set(USING_CLAN_GIT FALSE)
endif()


find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
set(BISON_EXTRA_COMPILE_FLAGS "-L c")
set(FLEX_EXTRA_COMPILE_FLAGS "")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(BISON_EXTRA_COMPILE_FLAGS
    "${BISON_EXTRA_COMPILE_FLAGS} -r all --report-file=${CMAKE_CURRENT_BINARY_DIR}/bison_debug")
  set(FLEX_EXTRA_COMPILE_FLAGS "-d")
endif()
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/autogen)
BISON_TARGET(acr_parser
  source/acr_parser.y ${CMAKE_CURRENT_BINARY_DIR}/autogen/acr_parser.c
  COMPILE_FLAGS ${BISON_EXTRA_COMPILE_FLAGS})
FLEX_TARGET(acr_scanner
  source/acr_scanner.l ${CMAKE_CURRENT_BINARY_DIR}/autogen/acr_scanner.c
  COMPILE_FLAGS ${FLEX_EXTRA_COMPILE_FLAGS})
ADD_FLEX_BISON_DEPENDENCY(acr_scanner acr_parser)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/autogen)

#///////////////////////////////////////////////////////////////////#
#                        COMPILATION OPTIONS                        #
#///////////////////////////////////////////////////////////////////#

set(COMMON_C_FLAGS "-Wall -Wextra -std=c99 -pedantic")

set(CMAKE_C_FLAGS_RELEASE "${COMMON_C_FLAGS} -O3")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_C_FLAGS} -O0 -g3")

# Use full RPATH on build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)
# Do not build with install RPATH
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# Set the RPATH when install
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# Only set RPATH if the installation directory is not a system directory
LIST(FIND
  CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib"
  isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
else()
  set(CMAKE_INSTALL_RPATH "")
endif()

#///////////////////////////////////////////////////////////////////#
#                         INCLUDE & SOURCES                         #
#///////////////////////////////////////////////////////////////////#

list(APPEND CURRENT_INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}/autogen/include")

if(USING_CLAN_GIT)
  list(APPEND CURRENT_INCLUDE_DIRECTORIES
    "${OSL_INCLUDE_DIRECTORIES}"
    "${CLAN_INCLUDE_DIRECTORIES}")
else()
  list(APPEND CURRENT_INCLUDE_DIRECTORIES
    "${CLAN_INCLUDE_DIRS}")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/acr/acr.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/autogen/include/acr/acr.h"
  IMMEDIATE @ONLY)

include_directories(${CURRENT_INCLUDE_DIRECTORIES})

list(APPEND ACR_LIBRARY_C_FILES
  ${FLEX_acr_scanner_OUTPUTS}
  ${BISON_acr_parser_OUTPUTS})
#  ${CMAKE_CURRENT_SOURCE_DIR}/source/)

#list(APPEND ACR_EXECUTABLE_C_FILES
#  ${CMAKE_CURRENT_SOURCE_DIR}/source/)

#///////////////////////////////////////////////////////////////////#
#                             LIBRARIES                             #
#///////////////////////////////////////////////////////////////////#

add_library(acr
  SHARED
  ${ACR_LIBRARY_C_FILES})
message(STATUS "Add acr library (shared)"
  " ${CMAKE_SHARED_LIBRARY_PREFIX}acr${CMAKE_SHARED_LIBRARY_SUFFIX}")

add_library(acr_static
  STATIC
  ${ACR_LIBRARY_C_FILES})
set_target_properties(acr_static PROPERTIES OUTPUT_NAME acr)
message(STATUS "Add acr library (static)"
  " ${CMAKE_STATIC_LIBRARY_PREFIX}acr${CMAKE_STATIC_LIBRARY_SUFFIX}")

#///////////////////////////////////////////////////////////////////#
#                            EXECUTABLE                             #
#///////////////////////////////////////////////////////////////////#

#add_executable(acr_exe ${ACR_EXECUTABLE_C_FILES})
#set_target_properties(clan_exe PROPERTIES OUTPUT_NAME "clan")
#if(USING_OSL_GIT)
#  target_link_libraries(clan_exe clan osl)
#else()
#  target_link_libraries(clan_exe clan ${OSL_LIBRARIES})
#endif()

#///////////////////////////////////////////////////////////////////#
#                           DOCUMENTATION                           #
#///////////////////////////////////////////////////////////////////#

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/doc")

#///////////////////////////////////////////////////////////////////#
#                               TEST                                #
#///////////////////////////////////////////////////////////////////#


#///////////////////////////////////////////////////////////////////#
#                             INSTALL                               #
#///////////////////////////////////////////////////////////////////#

#install(TARGETS acr LIBRARY
#  DESTINATION lib)
#install(TARGETS acr_static ARCHIVE
#  DESTINATION lib)
#install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
#  DESTINATION include FILES_MATCHING PATTERN "*.h")
#install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/autogen/include/"
#  DESTINATION include FILES_MATCHING PATTERN "*.h")
#install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/acr-config.cmake"
#  DESTINATION lib/)

#///////////////////////////////////////////////////////////////////#
#                               MISC                                #
#///////////////////////////////////////////////////////////////////#

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/misc/ycm_extra_conf.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/.ycm_extra_conf.py" IMMEDIATE @ONLY)
endif()

#///////////////////////////////////////////////////////////////////#
#                           USER INFOS                              #
#///////////////////////////////////////////////////////////////////#

message(STATUS "")
message(STATUS "******************************************************")
message(STATUS "")
message(STATUS "Configuration done.")
if (CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Build type is Release")
  message(STATUS "C compiler = ${CMAKE_C_COMPILER}")
  message(STATUS "C flags    = ${CMAKE_C_FLAGS_RELEASE}")
else()
  message(STATUS "Build type is Debug (default see CMAKE_BUILD_TYPE)")
  message(STATUS "C compiler = ${CMAKE_C_COMPILER}")
  message(STATUS "C flags    = ${CMAKE_C_FLAGS_DEBUG}")
endif()

message(STATUS "")
message(STATUS "You can execute:")
message(STATUS "    make          # To compile acr")
if (PYTHONINTERP_FOUND)
  message(STATUS "    make test     # To execute tests")
endif()
message(STATUS "    make install  # To install library, include and CMake module")
message(STATUS "                  # If you need root access:")
message(STATUS "                  #     sudo make install")
message(STATUS "                  #     su -c \"make install\"")
if(DOXYGEN_FOUND)
  message(STATUS "    make doxygen  # To generate the Doxygen")
endif()
if(texi2pdf_exe)
  message(STATUS "    make doc      # To generate the documentation")
endif()
