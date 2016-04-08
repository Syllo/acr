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

# Check functions avaliability
include(cmake/config-dep.cmake)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  include(cmake/compiler-flags.cmake)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

find_package(CLAN QUIET)
if(NOT CLAN_FOUND OR NOT "${CLAN_VERSION}" GREATER "0.8.0")
  if(NOT CLAN_FOUND)
    message(WARNING "CLAN is not installed")
  else()
    message(WARNING "CLAN version ${CLAN_VERSION} is too old")
  endif()
  message (STATUS "Fallback by trying to clone the repository from source")
  find_package(Git REQUIRED)
  execute_process(COMMAND
    ${GIT_EXECUTABLE} submodule update --init
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE CLAN_SUBMODULE_UPDATE)
  if (NOT CLAN_SUBMODULE_UPDATE EQUAL 0)
    message(FATAL_ERROR "Unable to clone CLAN, dependency not met")
  else()
    message(STATUS "CLAN successfully cloned, configuring")
    add_subdirectory(clan)
  endif()
elseif (NOT TARGET clan)
  add_library(clan INTERFACE IMPORTED)
  set_property(TARGET clan PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${CLAN_INCLUDE_DIRS})
  set_property(TARGET clan PROPERTY INTERFACE_LINK_LIBRARIES
    ${CLAN_LIBRARIES})
endif()

find_package(OSL QUIET)
if (OSL_FOUND AND NOT TARGET osl)
  add_library(osl INTERFACE IMPORTED)
  set_property(TARGET osl PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${OSL_INCLUDE_DIRS})
  set_property(TARGET osl PROPERTY INTERFACE_LINK_LIBRARIES
    ${OSL_LIBRARIES})
endif()

find_package(ISL REQUIRED)
if(ISL_VERSION AND "${ISL_VERSION}" LESS "0.16.0")
  message(FATAL_ERROR "ISL version >= 0.16 required but found ${ISL_VERSION}")
endif()
if (NOT TARGET isl)
  add_library(isl INTERFACE IMPORTED)
  set_property(TARGET isl PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${ISL_INCLUDE_DIRS})
  set_property(TARGET isl PROPERTY INTERFACE_LINK_LIBRARIES
    ${ISL_LIBRARIES})
endif()

find_package(CLOOG REQUIRED)
if (NOT TARGET cloog)
  add_library(cloog INTERFACE IMPORTED)
  set_property(TARGET cloog PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    ${CLOOG_INCLUDE_DIRS})
  set_property(TARGET cloog PROPERTY INTERFACE_LINK_LIBRARIES
    ${CLOOG_LIBRARIES})
  set_property(TARGET cloog PROPERTY INTERFACE_COMPILE_DEFINITIONS
    CLOOG_INT_GMP)
endif()

find_package(TCC QUIET)
if(TCC_FOUND)
  message(STATUS "Found TCC ${TCC_VERSION}")
else()
  message(WARNING "Tiny C Compiler not found, this may lower ACR performance")
endif()

#find_package(OpenMP QUIET)
if(OPENMP_FOUND)
  message(STATUS "Found OpenMP")
else()
  message(WARNING "OpenMP not found, this may lower ACR performances")
endif()

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
set(BISON_EXTRA_COMPILE_FLAGS "-L c")
set(FLEX_EXTRA_COMPILE_FLAGS " ")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(BISON_EXTRA_COMPILE_FLAGS
    "${BISON_EXTRA_COMPILE_FLAGS} -r all --report-file=${CMAKE_CURRENT_BINARY_DIR}/bison_debug")
  #set(FLEX_EXTRA_COMPILE_FLAGS "-d")
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

#set(CMAKE_THREAD_PREFER_PTHREAD_FLAG)
#find_package(Threads)

#///////////////////////////////////////////////////////////////////#
#                        COMPILATION OPTIONS                        #
#///////////////////////////////////////////////////////////////////#

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS_RELEASE "-O3" "${OpenMP_C_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3" "${OpenMP_C_FLAGS}" "-g")
set(CMAKE_C_FLAGS_DEBUG ${COMPILER_AVALIABLE_WARNINGS} "-O0" "-g"
  "${OpenMP_C_FLAGS}")
string(REPLACE ";" " " CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REPLACE ";" " " CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE ";" " " CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions()#-DACR_DEBUG)
endif()

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
#                             SOURCES                               #
#///////////////////////////////////////////////////////////////////#

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/acr/acr.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/autogen/include/acr/acr.h"
  IMMEDIATE @ONLY)

if(TCC_FOUND)
  set(TCC_PRESENT "#define TCC_PRESENT")
endif()
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/acr/acr_runtime_build.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/autogen/include/acr/acr_runtime_build.h"
  IMMEDIATE @ONLY)

list(APPEND ACR_RUNTIME_LIBRARY_C_FILES
  source/acr_runtime_build.c
  source/acr_runtime_data.c
  source/acr_runtime_threads.c
  source/acr_runtime_verify.c
  source/cloog_runtime.c
  source/isl_runtime.c
  source/osl_runtime.c)

list(APPEND ACR_LIBRARY_C_FILES
  source/acr_openscop.c
  source/gencode.c
  source/parser_utils.c
  source/pragma_struct.c
  source/print.c
  source/utils.c
  ${FLEX_acr_scanner_OUTPUTS}
  ${BISON_acr_parser_OUTPUTS})

list(APPEND ACR_EXECUTABLE_C_FILES
  source/acr.c)

#///////////////////////////////////////////////////////////////////#
#                             LIBRARIES                             #
#///////////////////////////////////////////////////////////////////#

add_library(acr
  SHARED
  ${ACR_LIBRARY_C_FILES})
message(STATUS "Add acr library (shared)"
  " ${CMAKE_SHARED_LIBRARY_PREFIX}acr${CMAKE_SHARED_LIBRARY_SUFFIX}")
target_link_libraries(acr
  PUBLIC
    cloog
    osl
  PRIVATE
    isl
    clan)
target_include_directories(acr
  PUBLIC
    include/
    "${CMAKE_CURRENT_BINARY_DIR}/autogen/include")
target_compile_definitions(acr
  PRIVATE
    _POSIX_C_SOURCE=200809L)

add_library(acrrun
  SHARED
  ${ACR_RUNTIME_LIBRARY_C_FILES})
message(STATUS "Add acr library (shared)"
  " ${CMAKE_SHARED_LIBRARY_PREFIX}acrrun${CMAKE_SHARED_LIBRARY_SUFFIX}")
target_link_libraries(acrrun
  PUBLIC
    cloog
    osl
    isl)
target_include_directories(acrrun
  PUBLIC
    include/
    "${CMAKE_CURRENT_BINARY_DIR}/autogen/include")
target_compile_definitions(acr
  PRIVATE
    _POSIX_C_SOURCE=200809L)

#///////////////////////////////////////////////////////////////////#
#                            EXECUTABLE                             #
#///////////////////////////////////////////////////////////////////#

add_executable(acr_exe ${ACR_EXECUTABLE_C_FILES})
set_target_properties(acr_exe PROPERTIES OUTPUT_NAME "acr")
target_link_libraries(acr_exe acr clan isl)
target_compile_definitions(acr_exe
  PRIVATE
    _POSIX_C_SOURCE=200809L)

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

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/misc/ycm_extra_conf.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/.ycm_extra_conf.py" IMMEDIATE @ONLY)

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
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Build type is Debug (default see CMAKE_BUILD_TYPE)")
  message(STATUS "C compiler = ${CMAKE_C_COMPILER}")
  message(STATUS "C flags    = ${CMAKE_C_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Build type is Debug (default see CMAKE_BUILD_TYPE)")
  message(STATUS "C compiler = ${CMAKE_C_COMPILER}")
  message(STATUS "C flags    = ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
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
