include(CheckCCompilerFlag)

if(CMAKE_BUILD_TYPE MATCHES Debug)

check_c_compiler_flag("-Wall" compiler_has_warning_wall)
if(compiler_has_warning_wall)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wall")
endif()
check_c_compiler_flag("-Wpedantic" compiler_has_warning_pedantic)
if(compiler_has_warning_pedantic)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wpedantic")
endif()
check_c_compiler_flag("-Wextra" compiler_has_warning_extra)
if(compiler_has_warning_extra)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wextra")
endif()
check_c_compiler_flag("-Waddress" compiler_has_warning_address)
if(compiler_has_warning_address)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Waddress")
endif()
check_c_compiler_flag("-Waggressive-loop-optimizations"
  compiler_has_warning_aggressive_loop_optimizations)
if(compiler_has_warning_aggressive_loop_optimizations)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Waggressive-loop-optimizations")
endif()
check_c_compiler_flag("-Wcast-qual" compiler_has_warning_cast_qual)
if(compiler_has_warning_cast_qual)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wcast-qual")
endif()
check_c_compiler_flag("-Wcast-align" compiler_has_warning_cast_align)
if(compiler_has_warning_cast_align)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wcast-align")
endif()
check_c_compiler_flag("-Wbad-function-cast"
  compiler_has_warning_bad_function_cast)
if(compiler_has_warning_bad_function_cast)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wbad-function-cast")
endif()
check_c_compiler_flag("-Wmissing-declarations"
  compiler_has_warning_missing_declaration)
if(compiler_has_warning_missing_declaration_warning)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wmissing-declarations")
endif()
check_c_compiler_flag("-Wmissing-parameter-type"
  compiler_has_warning_missing_parameter_type)
if(compiler_has_warning_missing_parameter_type)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wmissing-parameter-type")
endif()
check_c_compiler_flag("-Wmissing-prototypes"
  compiler_has_warning_missing_prototypes)
if(compiler_has_warning_missing_prototypes)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wmissing-prototypes")
endif()
check_c_compiler_flag("-Wnested-externs"
  compiler_has_warning_nested_externs)
if(compiler_has_warning_nested_externs)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wnested-externs")
endif()
check_c_compiler_flag("-Wold-style-declaration"
  compiler_has_warning_old_style_declaration)
if(compiler_has_warning_old_style_declaration)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wold-style-declaration")
endif()
check_c_compiler_flag("-Wold-style-definition"
  compiler_has_warning_old_style_definition)
if(compiler_has_warning_old_style_definition)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wold-style-definition")
endif()
check_c_compiler_flag("-Wstrict-prototypes"
  compiler_has_warning_strict_prototypes)
if(compiler_has_warning_strict_prototypes)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wstrict-prototypes")
endif()
check_c_compiler_flag("-Wpointer-sign" compiler_has_warning_pointer_sign)
if(compiler_has_warning_pointer_sign)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wpointer-sign")
endif()
check_c_compiler_flag("-Wdouble-promotion"
  compiler_has_warning_double_promotion)
if(compiler_has_warning_double_promotion)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wdouble-promotion")
endif()
check_c_compiler_flag("-Wuninitialized"
  compiler_has_warning_uninitialized)
if(compiler_has_warning_uninitialized)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wuninitialized")
endif()
check_c_compiler_flag("-Winit-self"
  compiler_has_warning_init_self)
if(compiler_has_warning_init_self)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Winit-self")
endif()
check_c_compiler_flag("-Wstrict-aliasing"
  compiler_has_warning_strict_aliasing)
if(compiler_has_warning_strict_aliasing)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wstrict-aliasing")
endif()
check_c_compiler_flag("-Wsuggest-attribute=const"
  compiler_has_warning_suggest_attribute_const)
if(compiler_has_warning_suggest_attribute_const)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wsuggest-attribute=const")
endif()
check_c_compiler_flag("-Wtrampolines" compiler_has_warning_trampolines)
if(compiler_has_warning_trampolines)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wtrampolines")
endif()
check_c_compiler_flag("-Wfloat-equal"
  compiler_has_warning_float_equal)
if(compiler_has_warning_float_equal)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wfloat-equal")
endif()
check_c_compiler_flag("-Wshadow" compiler_has_warning_shadow)
if(compiler_has_warning_shadow)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wshadow")
endif()
check_c_compiler_flag("-Wunsafe-loop-optimizations"
  compiler_has_warning_unsafe_loop_optimizations)
if(compiler_has_warning_unsafe_loop_optimizations)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wunsafe-loop-optimizations")
endif()
check_c_compiler_flag("-Wfloat-conversion"
  compiler_has_warning_float_conversion)
if(compiler_has_warning_float_conversion)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wfloat-conversion")
endif()
check_c_compiler_flag("-Wlogical-op"
  compiler_has_warning_logical_op)
if(compiler_has_warning_logical_op)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wlogical-op")
endif()
check_c_compiler_flag("-Wnormalized"
  compiler_has_warning_normalized)
if(compiler_has_warning_normalized)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wnormalized")
endif()
check_c_compiler_flag("-Wdisabled-optimization"
  compiler_has_warning_disabled_optimization)
if(compiler_has_warning_disabled_optimization)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wdisabled-optimization")
endif()
check_c_compiler_flag("-Whsa" compiler_has_warning_hsa)
if(compiler_has_warning_hsa)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Whsa")
endif()
check_c_compiler_flag("-Wconversion" compiler_has_warning_conversion)
if(compiler_has_warning_conversion)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wconversion")
endif()
check_c_compiler_flag("-Wunused-result" compiler_has_warning_unused_result)
if(compiler_has_warning_unused_result)
  list(APPEND COMPILER_AVALIABLE_WARNINGS
    "-Wunused-result")
endif()
#check_c_compiler_flag("-Wpadded" compiler_has_warning_padded)
#if(compiler_has_warning_padded)
  #list(APPEND COMPILER_AVALIABLE_WARNINGS
    #"-Wpadded")
#endif()

set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
check_c_compiler_flag("-fsanitize=address" compiler_has_address_sanitizer)
unset(CMAKE_REQUIRED_FLAGS)
if(compiler_has_address_sanitizer)
  set(COMPILER_ADDRESS_SANITIZER_FLAG "-fsanitize=address")
  # Nicer stack trace
  check_c_compiler_flag("-fno-omit-frame-pointer"
    compiler_has_no_omit_frame_pointer)
  if(compiler_has_no_omit_frame_pointer)
    set(COMPILER_ADDRESS_SANITIZER_FLAG
      "${COMPILER_ADDRESS_SANITIZER_FLAG} -fno-omit-frame-pointer")
  endif()
endif()

set(CMAKE_REQUIRED_FLAGS "-fsanitize=thread")
check_c_compiler_flag("-fsanitize=thread" compiler_has_thread_sanitizer)
unset(CMAKE_REQUIRED_FLAGS)
if(compiler_has_thread_sanitizer)
  set(COMPILER_THREAD_SANITIZER_FLAG "-fsanitize=thread")
endif()

endif(CMAKE_BUILD_TYPE MATCHES Debug)

set(CMAKE_REQUIRED_FLAGS "-flto")
check_c_compiler_flag("-flto" compiler_has_lto)
unset(CMAKE_REQUIRED_FLAGS)
if(compiler_has_lto)
  set(COMPILER_LTO_FLAG "-flto")
endif()

check_c_compiler_flag("-march=native" compiler_has_march_native)
if(compiler_has_march_native)
  set(COMPILER_MARCH_NATIVE "-march=native")
endif()
