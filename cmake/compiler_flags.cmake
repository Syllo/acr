include(CheckCCompilerFlag)

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
#check_c_compiler_flag("-Wconversion" compiler_has_warning_conversion)
#if(compiler_has_warning_conversion)
  #list(APPEND COMPILER_AVALIABLE_WARNINGS
    #"-Wconversion")
#endif()
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
