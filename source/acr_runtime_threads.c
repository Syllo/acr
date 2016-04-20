/*
 * Copyright (C) 2016 Maxime Schmitt
 *
 * ACR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "acr/acr_runtime_threads.h"

#include <dlfcn.h>

#include "acr/acr_runtime_verify.h"
#include "acr/cloog_runtime.h"

void* acr_runtime_monitoring_function(void* in_data) {
  struct acr_runtime_data *init_data =
    (struct acr_runtime_data*) in_data;
  struct acr_avaliable_functions functions = {
    .total_functions = 3,
  };
  functions.value = malloc(functions.total_functions * sizeof(*functions.value));
  for (size_t i = 0; i < functions.total_functions; ++i) {
    pthread_spin_init(&functions.value[i].lock, PTHREAD_PROCESS_PRIVATE);
    functions.value[i].type = acr_function_empty;
  }
  unsigned char *new_result =
    malloc(init_data->monitor_total_size * sizeof(*new_result));
  init_data->monitoring_function(new_result);
  functions.function_in_use = 0;

  pthread_t compile_thread;
  struct acr_runtime_threads_compile_data *cdata = malloc(sizeof(*cdata));
  cdata->monitor_result = new_result;
  cdata->rdata = init_data;
  cdata->functions = &functions;
  cdata->where_to_add = functions.function_in_use;
  pthread_create(&compile_thread, NULL, acr_runtime_compile_thread,
      (void*)cdata);
  pthread_detach(compile_thread);

  enum acr_avaliable_function_type type;
  while (init_data->monitor_thread_continue) {
    pthread_spin_lock(&functions.value[functions.function_in_use].lock);
    type = functions.value[functions.function_in_use].type;
    pthread_spin_unlock(&functions.value[functions.function_in_use].lock);

    if (type != acr_function_empty) {
      new_result =
        malloc(init_data->monitor_total_size * sizeof(*new_result));
      init_data->monitoring_function(new_result);

      bool still_ok = acr_verify_me(init_data->monitor_total_size,
          new_result, functions.value[functions.function_in_use].monitor_result);
      if (still_ok) {
        pthread_spin_lock(&init_data->alternative_lock);
        init_data->alternative_still_usable = init_data->usability_inital_value;
#ifdef TCC_PRESENT
        if (type > acr_function_tcc_in_memory) {
#endif
          init_data->alternative_function =
            functions.value[functions.function_in_use].cc_function;
#ifdef TCC_PRESENT
        } else {
          init_data->alternative_function =
            functions.value[functions.function_in_use].tcc_function;
        }
#endif
        pthread_spin_unlock(&init_data->alternative_lock);
        free(new_result);
      } else {
        // switch functions and command compilation
        functions.function_in_use =
          (functions.function_in_use + 1) % functions.total_functions;
#ifdef TCC_PRESENT
        do {
          pthread_spin_lock(&functions.value[functions.function_in_use].lock);
            type = functions.value[functions.function_in_use].type;
          pthread_spin_unlock(&functions.value[functions.function_in_use].lock);
        } while(type != acr_function_empty && type != acr_function_tcc_and_shared);
        if (type != acr_function_empty) {
          dlclose(functions.value[functions.function_in_use].
              compiler_specific.shared_obj_lib.dlhandle);
          tcc_delete(functions.value[functions.function_in_use].
              compiler_specific.tcc.state);
          free(functions.value[functions.function_in_use].monitor_result);
        }
#else
        if(functions.value[functions.function_in_use].type != acr_function_empty) {
          dlclose(functions.value[functions.function_in_use].
              compiler_specific.shared_obj_lib.dlhandle);
          free(functions.value[functions.function_in_use].monitor_result);
        }
#endif
        functions.value[functions.function_in_use].type = acr_function_empty;

        // New compilation
        cdata = malloc(sizeof(*cdata));
        cdata->monitor_result = new_result;
        cdata->rdata = init_data;
        cdata->functions = &functions;
        cdata->where_to_add = functions.function_in_use;
        pthread_create(&compile_thread, NULL, acr_runtime_compile_thread,
            (void*)cdata);
        pthread_detach(compile_thread);
      }
    }
    /*else {*/
      /*fprintf(stderr, "[Monitor] Waiting compilation\n");*/
    /*}*/
  }

  // Wait for compile thread to finish
  do {
    pthread_spin_lock(&functions.value[functions.function_in_use].lock);
    type = functions.value[functions.function_in_use].type;
    pthread_spin_unlock(&functions.value[functions.function_in_use].lock);
#ifdef TCC_PRESENT
  } while (type != acr_function_tcc_and_shared);
#else
  } while (type == acr_function_empty);
#endif
  // Clean all
  for (size_t i = 0; i < functions.total_functions; ++i) {
    pthread_spin_destroy(&functions.value[i].lock);
    switch(functions.value[i].type) {
      case acr_function_shared_object_lib:
        dlclose(functions.value[i].
            compiler_specific.shared_obj_lib.dlhandle);
        free(functions.value[i].monitor_result);
        break;
#ifdef TCC_PRESENT
      case acr_function_tcc_in_memory:
        break;
      case acr_function_tcc_and_shared:
        dlclose(functions.value[i].
            compiler_specific.shared_obj_lib.dlhandle);
        tcc_delete(functions.value[i].compiler_specific.tcc.state);
        free(functions.value[i].monitor_result);
        break;
#endif
      case acr_function_empty:
        break;
    }
  }
  free(functions.value);
  pthread_exit(NULL);
}

#ifdef TCC_PRESENT
static void* acr_runtime_compile_tcc(void* in_data) {
    struct acr_runtime_threads_compile_tcc *input_data =
      (struct acr_runtime_threads_compile_tcc *) in_data;
    input_data->functions->value[input_data->where_to_add].compiler_specific.tcc.state =
      acr_compile_with_tcc(input_data->generated_code);
    void *function =
      tcc_get_symbol(input_data->functions->value[input_data->where_to_add].
          compiler_specific.tcc.state, "acr_alternative_function");
    input_data->functions->value[input_data->where_to_add].tcc_function = function;
  input_data->using_data = false;
  pthread_spin_lock(&input_data->functions->value[input_data->where_to_add].lock);
  input_data->functions->value[input_data->where_to_add].type +=
    acr_function_tcc_in_memory;
  pthread_spin_unlock(&input_data->functions->value[input_data->where_to_add].lock);
  pthread_exit(NULL);
}
#endif

void* acr_runtime_compile_thread(void* in_data) {
  struct acr_runtime_threads_compile_data * input_data =
    (struct acr_runtime_threads_compile_data *) in_data;

  char* generated_code;
  char* file;
  size_t size_code;

  FILE* new_code = open_memstream(&generated_code, &size_code);
  fprintf(new_code, //"#include \"acr_required_definitions.h\"\n"
      "void acr_alternative_function%s {\n",
      input_data->rdata->function_prototype);
  acr_cloog_generate_alternative_code_from_input(new_code, input_data->rdata,
      input_data->monitor_result);
  fprintf(new_code, "}\n");
  fclose(new_code);

  input_data->functions->value[input_data->where_to_add].monitor_result =
    input_data->monitor_result;

#ifdef TCC_PRESENT
  struct acr_runtime_threads_compile_tcc *tcc_data = malloc(sizeof(*tcc_data));
  tcc_data->generated_code = generated_code;
  tcc_data->functions = input_data->functions;
  tcc_data->where_to_add = input_data->where_to_add;
  tcc_data->using_data = true;
  pthread_t tcc_thread;
  pthread_create(&tcc_thread, NULL, acr_runtime_compile_tcc, (void*)tcc_data);
#endif
  file =
    acr_compile_with_system_compiler(generated_code,
        input_data->rdata->num_compiler_flags,
        input_data->rdata->compiler_flags);
  if(!file) {
    fprintf(stderr, "Compiler error\n");
    exit(EXIT_FAILURE);
  }
  void *dlhandle = dlopen(file, RTLD_NOW);
  if(!dlhandle) {
    fprintf(stderr, "dlopen error: %s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  input_data->functions->value[input_data->where_to_add].compiler_specific.shared_obj_lib.dlhandle = dlhandle;
  void *function = dlsym(dlhandle, "acr_alternative_function");
  if(!function) {
    fprintf(stderr, "dlsym error: %s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  input_data->functions->value[input_data->where_to_add].cc_function = function;

  pthread_spin_lock(&input_data->functions->value[input_data->where_to_add].lock);
  input_data->functions->value[input_data->where_to_add].type +=
    acr_function_shared_object_lib;
  pthread_spin_unlock(&input_data->functions->value[input_data->where_to_add].lock);
  if (file) {
    if(unlink(file) == -1) {
      perror("unlink");
      exit(EXIT_FAILURE);
    }
    free(file);
  }
#ifdef TCC_PRESENT
  pthread_join(tcc_thread, NULL);
  free(tcc_data);
#endif
  free(in_data);
  free(generated_code);

  pthread_exit(NULL);
}
