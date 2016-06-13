include(CheckSymbolExists)
include(CheckFunctionExists)

set(CMAKE_REQUIRED_DEFINITIONS "-D_POSIX_C_SOURCE=200809L")

check_symbol_exists("fileno" "stdio.h" HAVE_FILENO)
check_function_exists("fileno" HAVE_FUN_FILENO)

check_symbol_exists("open_memstream" "stdio.h" HAVE_OPEN_MEMSTREAM)
check_function_exists("open_memstream" HAVE_FUN_OPEN_MEMSTREAM)

check_symbol_exists("fmemopen" "stdio.h" HAVE_FMEMOPEN)
check_function_exists("fmemopen" HAVE_FUN_FMEMOPEN)

check_symbol_exists("dup2" "unistd.h" HAVE_DUP2)
check_function_exists("dup2" HAVE_FUN_DUP2)

check_symbol_exists("pipe" "unistd.h" HAVE_PIPE)
check_function_exists("pipe" HAVE_FUN_PIPE)

check_symbol_exists("fork" "unistd.h" HAVE_FORK)
check_function_exists("fork" HAVE_FUN_FORK)

check_symbol_exists("execv" "unistd.h" HAVE_EXECV)
check_function_exists("execv" HAVE_FUN_EXECV)

check_symbol_exists("unlink" "unistd.h" HAVE_UNLINK)
check_function_exists("unlink" HAVE_FUN_UNLINK)

check_symbol_exists("mkstemp" "stdlib.h" HAVE_MKSTEMP)
check_function_exists("mkstemp" HAVE_FUN_MKSTEMP)

check_symbol_exists("waitpid" "sys/wait.h" HAVE_WAITPID)
check_function_exists("waitpid" HAVE_FUN_WAITPID)

check_symbol_exists("sysconf" "unistd.h" HAVE_SYSCONF)
check_function_exists("sysconf" HAVE_FUN_SYSCONF)

if(NOT HAVE_FILENO OR NOT HAVE_FUN_FILENO)
  message(FATAL_ERROR "Function fileno() required")
endif()

if(NOT HAVE_OPEN_MEMSTREAM OR NOT HAVE_FUN_OPEN_MEMSTREAM)
  message(FATAL_ERROR "Function open_memstream() required")
endif()

if(NOT HAVE_FMEMOPEN OR NOT HAVE_FUN_FMEMOPEN)
  message(FATAL_ERROR "Function fmemopen() required")
endif()

if(NOT HAVE_FORK OR NOT HAVE_FUN_FORK)
  message(FATAL_ERROR "Function fork() required")
endif()

if(NOT HAVE_DUP2 OR NOT HAVE_FUN_DUP2)
  message(FATAL_ERROR "Function dup2() required")
endif()

if(NOT HAVE_PIPE OR NOT HAVE_FUN_PIPE)
  message(FATAL_ERROR "Function pipe() required")
endif()

if(NOT HAVE_EXECV OR NOT HAVE_FUN_EXECV)
  message(FATAL_ERROR "Function execv() required")
endif()

if(NOT HAVE_UNLINK OR NOT HAVE_FUN_UNLINK)
  message(FATAL_ERROR "Function unlink() required")
endif()

if(NOT HAVE_MKSTEMP OR NOT HAVE_FUN_MKSTEMP)
  message(FATAL_ERROR "Function mkstemp() required")
endif()

if(NOT HAVE_WAITPID OR NOT HAVE_FUN_WAITPID)
  message(FATAL_ERROR "Function waitpid() required")
endif()

if(NOT HAVE_SYSCONF OR NOT HAVE_FUN_SYSCONF)
  message(FATAL_ERROR "Function waitpid() required")
endif()
