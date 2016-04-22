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

void* acr_runtime_monitoring_function(void *in_data) {
  struct acr_monitoring_computation *input_data =
    (struct acr_monitoring_computation*) in_data;

  unsigned char *monitor_result =
    malloc(input_data->monitor_result_size * sizeof(*monitor_result));

  void (*const monitoring_function)(unsigned char*) =
    input_data->monitoring_function;

  for(;!input_data->end_yourself;) {
    monitoring_function(monitor_result);
    pthread_spin_lock(&input_data->spinlock);
    if (input_data->current_valid_computation == NULL) {
      input_data->current_valid_computation = monitor_result;
      monitor_result = input_data->scrap_values;
    } else {
      unsigned char *tempswitch = input_data->current_valid_computation;;
      input_data->current_valid_computation = monitor_result;
      monitor_result = tempswitch;
    }
    pthread_spin_unlock(&input_data->spinlock);
  }
  free(monitor_result);

  pthread_exit(NULL);
}

void* acr_verification_and_coordinator_function(void *in_data) {
  struct acr_runtime_data *init_data =
    (struct acr_runtime_data*) in_data;

  const size_t num_functions = 3;

  struct acr_avaliable_functions functions = {
    .total_functions = num_functions,
  };
  functions.value = malloc(functions.total_functions * sizeof(*functions.value));
  for (size_t i = 0; i < functions.total_functions; ++i) {
    pthread_spin_init(&functions.value[i].lock, PTHREAD_PROCESS_PRIVATE);
    functions.value[i].type = acr_function_empty;
    functions.value[i].monitor_result =
      malloc(init_data->monitor_total_size *
          sizeof(*functions.value[i].monitor_result));
  }

  // Monitoring thread
  pthread_t monitoring_thread;
  struct acr_monitoring_computation monitor_data=
      { .monitoring_function = init_data->monitoring_function,
        .current_valid_computation = NULL,
        .monitor_result_size = init_data->monitor_total_size,
        .end_yourself = false,
      };
  pthread_spin_init(&monitor_data.spinlock, PTHREAD_PROCESS_PRIVATE);
  monitor_data.scrap_values =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data.scrap_values));
  pthread_create(&monitoring_thread, NULL, acr_runtime_monitoring_function,
      (void*) &monitor_data);

  // Compile threads
  const size_t num_compilation_threads = 1;
  pthread_t *compile_threads;
  struct acr_runtime_threads_compile_data *compile_threads_data;
  pthread_t cloog_thread;
  struct acr_runtime_threads_cloog_gencode cloog_thread_data;

  compile_threads =
    malloc(num_compilation_threads * sizeof(*compile_threads));
  compile_threads_data =
    malloc(num_compilation_threads *
        sizeof(*compile_threads_data));
  for (size_t i = 0; i < num_compilation_threads; ++i) {
    pthread_mutex_init(&compile_threads_data[i].mutex, NULL);
    pthread_cond_init(&compile_threads_data[i].waking_up, NULL);
    compile_threads_data[i].cflags = init_data->compiler_flags;
    compile_threads_data[i].num_cflags = init_data->num_compiler_flags;
    compile_threads_data[i].end_yourself = false;
    compile_threads_data[i].compile_something = true;
    pthread_create(&compile_threads[i], NULL,
        acr_runtime_compile_thread, (void*)&compile_threads_data[i]);
  }

  // Cloog thread
  pthread_mutex_init(&cloog_thread_data.mutex, NULL);
  pthread_cond_init(&cloog_thread_data.waking_up, NULL);
  cloog_thread_data.end_yourself = false;
  cloog_thread_data.generate_function = true;
  cloog_thread_data.functions = &functions;
  pthread_create(&cloog_thread, NULL, acr_cloog_generate_code_from_alt,
      (void*)&cloog_thread_data);

  unsigned char *valid_monitor_result = NULL;
    /*malloc(init_data->monitor_total_size * sizeof(*valid_monitor_result));*/

  size_t function_in_use = 0;

  size_t current_compile_thread = 0;

  // Start gencode

  // Wait for one valid monitor data
  do {
    pthread_spin_lock(&monitor_data.spinlock);
    valid_monitor_result = monitor_data.current_valid_computation;
    pthread_spin_unlock(&monitor_data.spinlock);
  } while (valid_monitor_result == NULL);

  pthread_spin_lock(&monitor_data.spinlock);
  valid_monitor_result = monitor_data.current_valid_computation;
  monitor_data.current_valid_computation = NULL;
  monitor_data.scrap_values =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data.scrap_values));
  pthread_spin_unlock(&monitor_data.spinlock);

  // Gen with CLooG the first valid function
  pthread_mutex_lock(&cloog_thread_data.mutex);
  while(cloog_thread_data.generate_function) {
    pthread_cond_wait(&cloog_thread_data.waking_up, &cloog_thread_data.mutex);
  }
  cloog_thread_data.where_to_add = function_in_use;
  cloog_thread_data.monitor_result = valid_monitor_result;
  cloog_thread_data.generate_function = true;
  pthread_mutex_unlock(&cloog_thread_data.mutex);

  // CLooG it's your time to shine
  pthread_cond_broadcast(&cloog_thread_data.waking_up);

  enum acr_avaliable_function_type type;
  do {
    pthread_spin_lock(&functions.value[function_in_use].lock);
    type = functions.value[function_in_use].type;
    pthread_spin_unlock(&functions.value[function_in_use].lock);
  } while (type != acr_function_finished_cloog_gen);

  // From now on at least one function has a valid monitor result

  while (init_data->monitor_thread_continue) {
    // Get last valid monitor result
    pthread_spin_lock(&monitor_data.spinlock);

    if (monitor_data.current_valid_computation != NULL) { // If more recent get those
      monitor_data.scrap_values = valid_monitor_result;
      valid_monitor_result = monitor_data.current_valid_computation;
      monitor_data.current_valid_computation = NULL;
    }

    pthread_spin_unlock(&monitor_data.spinlock);
    // Test current function validity
    if (acr_verify_me(init_data->monitor_total_size,
          functions.value[function_in_use].monitor_result,
          valid_monitor_result)) { // Valid function
      pthread_spin_lock(&functions.value[function_in_use].lock);
      type = functions.value[function_in_use].type;
      pthread_spin_unlock(&functions.value[function_in_use].lock);
      size_t next_compile_thread;
      switch(type) {
        case acr_function_finished_cloog_gen:  // missing C compilation
          next_compile_thread = current_compile_thread;
          do {
            next_compile_thread = (next_compile_thread+1) %
              num_compilation_threads;
            pthread_mutex_lock(&compile_threads_data[next_compile_thread].mutex);
            if (compile_threads_data[next_compile_thread].compile_something
                == false) { // thread ok compilation
              functions.value[function_in_use].type = acr_function_started_compilation;
              compile_threads_data[next_compile_thread].compile_something = true;
              compile_threads_data[next_compile_thread].where_to_add = function_in_use;
              pthread_mutex_unlock(&compile_threads_data[next_compile_thread].mutex);
              pthread_cond_broadcast(&compile_threads_data[next_compile_thread].waking_up);
              current_compile_thread = next_compile_thread;
              break;
            }
            pthread_mutex_unlock(&compile_threads_data[next_compile_thread].mutex);
          } while (current_compile_thread != next_compile_thread);
          break;
        case acr_function_started_compilation: // - Compilation started
                                               //   but not finished
          break;
        case acr_function_tcc_in_memory:
          pthread_spin_lock(&init_data->alternative_lock);
          init_data->alternative_function = functions.value[function_in_use].tcc_function;
          init_data->alternative_still_usable = init_data->usability_inital_value;
          pthread_spin_unlock(&init_data->alternative_lock);
          break;
        case acr_function_shared_object_lib:
        case acr_function_tcc_and_shared:
          pthread_spin_lock(&init_data->alternative_lock);
          init_data->alternative_function = functions.value[function_in_use].cc_function;
          init_data->alternative_still_usable = init_data->usability_inital_value;
          pthread_spin_unlock(&init_data->alternative_lock);
          break;
        case acr_function_empty: // Cloog has not finished
          break;
      }
    } else {
      // Function no more suitable
      pthread_spin_lock(&init_data->alternative_lock);
      init_data->alternative_still_usable = 0;
      pthread_spin_unlock(&init_data->alternative_lock);

      // Round robin function
      function_in_use = (function_in_use + 1) % num_functions;

      pthread_mutex_lock(&cloog_thread_data.mutex);
      while(cloog_thread_data.generate_function) { // Previous CLooG not finished
        pthread_cond_wait(&cloog_thread_data.waking_up, &cloog_thread_data.mutex);
      }
      // CLooG with last valid result
      pthread_spin_lock(&monitor_data.spinlock);
      if (monitor_data.current_valid_computation != NULL) { // If more recent get those
        monitor_data.scrap_values = valid_monitor_result;
        valid_monitor_result = monitor_data.current_valid_computation;
        monitor_data.current_valid_computation = NULL;
      }
      pthread_spin_unlock(&monitor_data.spinlock);

      cloog_thread_data.where_to_add = function_in_use;
      cloog_thread_data.monitor_result = valid_monitor_result;
      cloog_thread_data.generate_function = true;
      pthread_mutex_unlock(&cloog_thread_data.mutex);
      pthread_cond_broadcast(&cloog_thread_data.waking_up);
      size_t considered_old_function = function_in_use == 0 ?
        num_functions - 2 : function_in_use - 1;
      do {
        pthread_spin_lock(&functions.value[function_in_use].lock);
        type = functions.value[function_in_use].type;
        pthread_spin_unlock(&functions.value[function_in_use].lock);
        if (type == acr_function_finished_cloog_gen)
          break;
        else {
          if (considered_old_function == function_in_use)
            break;
        }
      } while (1);

    }
  }

  // Wait for compile thread to finish
  for (size_t i = 0; i < num_compilation_threads; ++i) {
    pthread_mutex_lock(&compile_threads_data[i].mutex);
    compile_threads_data[i].end_yourself = true;
    pthread_mutex_unlock(&compile_threads_data[i].mutex);
    pthread_cond_broadcast(&compile_threads_data[i].waking_up);
  }
  pthread_mutex_lock(&cloog_thread_data.mutex);
  cloog_thread_data.end_yourself = true;
  pthread_mutex_unlock(&cloog_thread_data.mutex);
  pthread_cond_broadcast(&cloog_thread_data.waking_up);
  monitor_data.end_yourself = true;

  for (size_t i = 0; i < num_compilation_threads; ++i) {
    pthread_join(compile_threads[i], NULL);
    pthread_mutex_destroy(&compile_threads_data[i].mutex);
    pthread_cond_destroy(&compile_threads_data[i].waking_up);
  }
  pthread_join(cloog_thread, NULL);
  pthread_mutex_destroy(&cloog_thread_data.mutex);
  pthread_cond_destroy(&cloog_thread_data.waking_up);
  pthread_join(monitoring_thread, NULL);
  pthread_spin_destroy(&monitor_data.spinlock);
  free(monitor_data.current_valid_computation);

  // Clean all
  for (size_t i = 0; i < functions.total_functions; ++i) {
    free(functions.value[i].monitor_result);
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
      case acr_function_finished_cloog_gen:
      case acr_function_started_compilation:
      case acr_function_empty:
        break;
    }
  }
  free(functions.value);
  pthread_exit(NULL);
}

void* acr_cloog_generate_code_from_alt(void* in_data) {
  struct acr_runtime_threads_cloog_gencode * input_data =
    (struct acr_runtime_threads_cloog_gencode *) in_data;

  struct acr_avaliable_functions * const functions = input_data->functions;
  const size_t monitor_total_size = input_data->monitor_total_size;

  for (;;) {
    char* generated_code;
    size_t size_code;
    size_t where_to_add;
    unsigned char *monitor_result;
    bool has_to_stop;

    pthread_mutex_lock(&input_data->mutex);

    pthread_cond_broadcast(&input_data->waking_up);
    input_data->generate_function = false;
    while(!input_data->generate_function || !input_data->end_yourself) {
      pthread_cond_wait(&input_data->waking_up, &input_data->mutex);
    }
    has_to_stop = input_data->end_yourself;
    monitor_result = input_data->monitor_result;
    where_to_add = input_data->where_to_add;

    pthread_mutex_unlock(&input_data->mutex);

    if (has_to_stop)
      break;

    FILE* new_code = open_memstream(&generated_code, &size_code);
    fprintf(new_code, //"#include \"acr_required_definitions.h\"\n"
        "void acr_alternative_function%s {\n",
        input_data->rdata->function_prototype);
    acr_cloog_generate_alternative_code_from_input(new_code, input_data->rdata,
        monitor_result);
    fprintf(new_code, "}\n");
    fclose(new_code);

    memcpy(functions->value[where_to_add].monitor_result, monitor_result,
        monitor_total_size);
    pthread_spin_lock(&functions->value[where_to_add].lock);
    functions->value[where_to_add].type =
      acr_function_finished_cloog_gen;
    functions->value[where_to_add].generated_code =
      generated_code;
    pthread_spin_unlock(&functions->value[where_to_add].lock);
  }

  pthread_exit(NULL);
}

#ifdef TCC_PRESENT
static void* acr_runtime_compile_tcc(void* in_data) {
  struct acr_runtime_threads_compile_tcc *input_data =
    (struct acr_runtime_threads_compile_tcc *) in_data;

  struct acr_avaliable_functions * const functions = input_data->functions;

  size_t function_index;
  bool has_to_stop;
  for (;;) {

    pthread_mutex_lock(&input_data->mutex);
    input_data->compile_something = false;
    pthread_cond_broadcast(&input_data->waking_up);
    while(!input_data->compile_something || !input_data->end_yourself) {
      pthread_cond_wait(&input_data->waking_up, &input_data->mutex);
    }
    has_to_stop = input_data->end_yourself;
    function_index = input_data->where_to_add;
    pthread_mutex_unlock(&input_data->mutex);

    if (has_to_stop)
      break;

    TCCState *tccstate =
      acr_compile_with_tcc(input_data->generated_code);
    void *function =
      tcc_get_symbol(tccstate, "acr_alternative_function");

    pthread_spin_lock(&functions->value[function_index].lock);

    functions->value[function_index].compiler_specific.tcc.state =
      tccstate;
    functions->value[function_index].tcc_function = function;
    functions->value[function_index].type =
     functions->value[function_index].type == acr_function_shared_object_lib ?
     acr_function_tcc_and_shared : acr_function_tcc_in_memory;
    functions->value[function_index].type +=
      acr_function_tcc_in_memory;

    pthread_spin_unlock(&functions->value[function_index].lock);
  }
  pthread_exit(NULL);
}
#endif

void* acr_runtime_compile_thread(void* in_data) {
  struct acr_runtime_threads_compile_data * input_data =
    (struct acr_runtime_threads_compile_data *) in_data;

  pthread_t tcc_thread;
  char* file;
  struct acr_avaliable_functions * const functions = input_data->functions;

#ifdef TCC_PRESENT
  struct acr_runtime_threads_compile_tcc tcc_data;
  tcc_data.compile_something = true;
  tcc_data.end_yourself = false;
  pthread_mutex_init(&tcc_data.mutex, NULL);
  pthread_cond_init(&tcc_data.waking_up, NULL);
  pthread_create(&tcc_thread, NULL, acr_runtime_compile_tcc, (void*)&tcc_data);
  tcc_data.functions = functions;
#endif

  for (;;) {
    char *code_to_compile;
    bool has_to_stop;
    size_t function_index;

    pthread_mutex_lock(&input_data->mutex);

    input_data->compile_something = false;
    while(!input_data->compile_something || !input_data->end_yourself) {
      pthread_cond_wait(&input_data->waking_up, &input_data->mutex);
    }
    has_to_stop = input_data->end_yourself;
    function_index = input_data->where_to_add;
    pthread_mutex_unlock(&input_data->mutex);
    code_to_compile = functions->value[function_index].generated_code;

    if (has_to_stop)
      break;

#ifdef TCC_PRESENT
    tcc_data.generated_code = code_to_compile;
    tcc_data.where_to_add = function_index;
    pthread_mutex_lock(&tcc_data.mutex);
    while (tcc_data.compile_something == true) {
      pthread_cond_wait(&tcc_data.waking_up, &tcc_data.mutex);
    }
    tcc_data.compile_something = true;
    pthread_mutex_unlock(&tcc_data.mutex);
    pthread_cond_broadcast(&tcc_data.waking_up);
#endif
    file =
      acr_compile_with_system_compiler(code_to_compile,
          input_data->num_cflags,
          input_data->cflags);
    if(!file) {
      fprintf(stderr, "Compiler error\n");
      exit(EXIT_FAILURE);
    }
    void *dlhandle = dlopen(file, RTLD_NOW);
    if(!dlhandle) {
      fprintf(stderr, "dlopen error: %s\n", dlerror());
      exit(EXIT_FAILURE);
    }
    void *function = dlsym(dlhandle, "acr_alternative_function");
    if(!function) {
      fprintf(stderr, "dlsym error: %s\n", dlerror());
      exit(EXIT_FAILURE);
    }

    pthread_spin_lock(&functions->value[function_index].lock);
    functions->value[function_index].
      compiler_specific.shared_obj_lib.dlhandle = dlhandle;
    functions->value[function_index].cc_function = function;
    functions->value[function_index].type =
#ifdef TCC_PRESENT
     functions->value[function_index].type == acr_function_tcc_in_memory ?
     acr_function_tcc_and_shared : acr_function_shared_object_lib;
#else
     acr_function_shared_object_lib;
#endif
    pthread_spin_unlock(&functions->value[function_index].lock);
    if(unlink(file) == -1) {
      perror("unlink");
      exit(EXIT_FAILURE);
    }
    free(file);
  }
#ifdef TCC_PRESENT
  pthread_mutex_lock(&tcc_data.mutex);
  tcc_data.end_yourself = true;
  pthread_mutex_unlock(&tcc_data.mutex);
  pthread_cond_broadcast(&tcc_data.waking_up);
  pthread_join(tcc_thread, NULL);
  pthread_mutex_destroy(&tcc_data.mutex);
  pthread_cond_destroy(&tcc_data.waking_up);
#endif
  free(in_data);

  pthread_exit(NULL);
}
