include(CheckSymbolExists)
include(CheckFunctionExists)

set(CMAKE_REQUIRED_DEFINITIONS "-D_POSIX_C_SOURCE=200809L")

check_symbol_exists("fileno" "stdio.h" HAVE_FILENO)
check_symbol_exists("open_memstream" "stdio.h" HAVE_OPEN_MEMSTREAM)
check_symbol_exists("fmemopen" "stdio.h" HAVE_FMEMOPEN)

check_function_exists("fileno" HAVE_FUN_FILENO)
check_function_exists("open_memstream" HAVE_FUN_OPEN_MEMSTREAM)
check_function_exists("fmemopen" HAVE_FUN_FMEMOPEN)

if(NOT HAVE_FILENO OR NOT HAVE_FUN_FILENO)
  message(FATAL_ERROR "Function fileno() required")
endif()

if(NOT HAVE_OPEN_MEMSTREAM OR NOT HAVE_FUN_OPEN_MEMSTREAM )
  message(FATAL_ERROR "Function open_memstream() required")
endif()

if(NOT HAVE_FMEMOPEN OR NOT HAVE_FUN_FMEMOPEN )
  message(FATAL_ERROR "Function fmemopen() required")
endif()
