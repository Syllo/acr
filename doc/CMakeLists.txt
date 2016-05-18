
#///////////////////////////////////////////////////////////////////#
#                           DOCUMENTATION                           #
#///////////////////////////////////////////////////////////////////#

find_package(Doxygen)
if(DOXYGEN_FOUND)
  if(NOT TARGET doxygen)
    add_custom_target(doxygen)
  endif()

  set(ACR_DOXYGEN_DOXYFILE "${AUTOGEN_OUTPUT_PATH}/Doxyfile")
  set(ACR_DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/doc")
  set(ACR_DOXYGEN_INPUT_DIRS
    "${PROJECT_SOURCE_DIR}/source\
    ${PROJECT_SOURCE_DIR}/include\
    ${PROJECT_SOURCE_DIR}/README.md")
  set(ACR_DOXYGEN_OUTPUT_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/html/index.xhtml"
    "${CMAKE_CURRENT_BINARY_DIR}/latex/Makefile")
  if(CMAKE_BUILD_TYPE MATCHES Release)
    set(ACR_DOXYGEN_WARNINGS "NO")
  else()
    set(ACR_DOXYGEN_WARNINGS "YES")
  endif()
  if(DOXYGEN_DOT_FOUND)
    message(STATUS "DOT found, generating Doxygen DOT graphics")
    set(DOXYGEN_HAVE_DOT "YES")
  else()
    set(DOXYGEN_HAVE_DOT "NO")
  endif()

  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" IMMEDIATE @ONLY)

  add_custom_command(OUTPUT ${ACR_DOXYGEN_OUTPUT_FILES}
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    COMMENT "Generating Doxygen doc")

  add_custom_target(acr-doxygen
    DEPENDS ${ACR_DOXYGEN_OUTPUT_FILES})

  add_dependencies(doxygen acr-doxygen)
else()
  message (STATUS "Doxygen not found: API documentation can not be built")
endif()
