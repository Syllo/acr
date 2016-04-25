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
      input_data->scrap_values = input_data->current_valid_computation;;
      input_data->current_valid_computation = monitor_result;
      monitor_result = input_data->scrap_values;
    }
    pthread_spin_unlock(&input_data->spinlock);
  }

  free(monitor_result);
  if (input_data->current_valid_computation) {
    free(input_data->current_valid_computation);
  } else {
    free(input_data->scrap_values);
  }

  pthread_exit(NULL);
}

void* acr_verification_and_coordinator_function(void *in_data) {
  struct acr_runtime_data *init_data =
    (struct acr_runtime_data*) in_data;

  fprintf(stderr, "Coordinator start\n");
  const size_t num_functions = 10;

  struct acr_avaliable_functions functions = {
    .total_functions = num_functions,
  };
  functions.value = malloc(functions.total_functions * sizeof(*functions.value));
  functions.function_priority = malloc(functions.total_functions *
      sizeof(*functions.function_priority));
  for (size_t i = 0; i < functions.total_functions; ++i) {
    pthread_spin_init(&functions.value[i].lock, PTHREAD_PROCESS_PRIVATE);
#ifdef TCC_PRESENT
    functions.value[i].tcc_function = NULL;
#endif
    functions.value[i].cc_function = NULL;
    functions.value[i].type = acr_function_empty;
    functions.value[i].monitor_result =
      malloc(init_data->monitor_total_size *
          sizeof(*functions.value[i].monitor_result));
    functions.function_priority[i] = &functions.value[i];
  }

  // Monitoring thread
  pthread_t monitoring_thread;
  struct acr_monitoring_computation monitor_data =
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
  const size_t num_compilation_threads = 4;
  struct acr_runtime_threads_compile_data compile_threads_data;

  pthread_t *compile_threads =
    malloc(num_compilation_threads * sizeof(*compile_threads));
  pthread_mutex_init(&compile_threads_data.mutex, NULL);
  pthread_cond_init(&compile_threads_data.compiler_thread_sleep, NULL);
  pthread_cond_init(&compile_threads_data.coordinator_sleep, NULL);
  compile_threads_data.cflags = init_data->compiler_flags;
  compile_threads_data.num_cflags = init_data->num_compiler_flags;
  compile_threads_data.end_yourself = false;
  compile_threads_data.compile_something = false;
  compile_threads_data.num_threads = num_compilation_threads;
  compile_threads_data.num_threads_compiling = num_compilation_threads;
  for (size_t i = 0; i < num_compilation_threads; ++i) {
    pthread_create(&compile_threads[i], NULL,
        acr_runtime_compile_thread, (void*)&compile_threads_data);
  }

  // Cloog thread
  const size_t num_cloog_threads = 1;
  pthread_t *cloog_threads =
    malloc(num_cloog_threads * sizeof(*compile_threads));
  struct acr_runtime_threads_cloog_gencode cloog_thread_data;
  pthread_mutex_init(&cloog_thread_data.mutex, NULL);
  pthread_cond_init(&cloog_thread_data.coordinator_sleep, NULL);
  pthread_cond_init(&cloog_thread_data.compiler_thread_sleep, NULL);
  cloog_thread_data.end_yourself = false;
  cloog_thread_data.generate_function = false;
  cloog_thread_data.num_threads = num_cloog_threads;
  cloog_thread_data.num_threads_compiling = num_cloog_threads;
  cloog_thread_data.rdata = init_data;
  cloog_thread_data.monitor_total_size = init_data->monitor_total_size;
  for (size_t i = 0; i < num_cloog_threads; ++i) {
    pthread_create(&cloog_threads[i], NULL, acr_cloog_generate_code_from_alt,
        (void*)&cloog_thread_data);
  }

  unsigned char *valid_monitor_result = NULL;
  size_t function_in_use = 0;

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
  while(cloog_thread_data.num_threads == cloog_thread_data.num_threads_compiling) {
    pthread_cond_wait(&cloog_thread_data.coordinator_sleep, &cloog_thread_data.mutex);
  }
  cloog_thread_data.where_to_add = functions.function_priority[function_in_use];
  cloog_thread_data.monitor_result = valid_monitor_result;
  cloog_thread_data.generate_function = true;
  pthread_mutex_unlock(&cloog_thread_data.mutex);

  // CLooG it's your time to shine
  pthread_cond_signal(&cloog_thread_data.compiler_thread_sleep);

  enum acr_avaliable_function_type type;
  do {
    pthread_spin_lock(&functions.function_priority[function_in_use]->lock);
    type = functions.function_priority[function_in_use]->type;
    pthread_spin_unlock(&functions.function_priority[function_in_use]->lock);
  } while (type != acr_function_finished_cloog_gen);

  // From now on at least one function has a valid monitor result

  while (init_data->monitor_thread_continue) {
    // Get last valid monitor result
    pthread_spin_lock(&monitor_data.spinlock);
    if (monitor_data.current_valid_computation != NULL) { // Get most recent update
      monitor_data.scrap_values = valid_monitor_result;
      valid_monitor_result = monitor_data.current_valid_computation;
      monitor_data.current_valid_computation = NULL;
      pthread_spin_unlock(&monitor_data.spinlock);
    } else {
      pthread_spin_unlock(&monitor_data.spinlock);
      continue;
    }

    // Test current function validity
    if (acr_verify_me(init_data->monitor_total_size,
          functions.function_priority[function_in_use]->monitor_result,
          valid_monitor_result)) { // Valid function
      pthread_spin_lock(&functions.function_priority[function_in_use]->lock);
      type = functions.function_priority[function_in_use]->type;
      pthread_spin_unlock(&functions.function_priority[function_in_use]->lock);
      switch(type) {
        case acr_function_finished_cloog_gen:  // missing C compilation
          pthread_mutex_lock(&compile_threads_data.mutex);
          while(compile_threads_data.num_threads ==
              compile_threads_data.num_threads_compiling) {
            pthread_cond_wait(&compile_threads_data.coordinator_sleep,
                &compile_threads_data.mutex);
          }
          functions.function_priority[function_in_use]->type =
            acr_function_started_compilation;
          compile_threads_data.compile_something = true;
          compile_threads_data.where_to_add =
            functions.function_priority[function_in_use];
          pthread_cond_signal(&compile_threads_data.compiler_thread_sleep);
          pthread_mutex_unlock(&compile_threads_data.mutex);
          break;
        case acr_function_started_compilation: // - Waiting compilation
          break;
#ifdef TCC_PRESENT
        case acr_function_tcc_in_memory: // Fast compilation finished
          pthread_spin_lock(&init_data->alternative_lock);
          init_data->alternative_function =
            functions.function_priority[function_in_use]->tcc_function;
          init_data->alternative_still_usable =
            init_data->usability_inital_value;
          pthread_spin_unlock(&init_data->alternative_lock);
          init_data->current_monitoring_data =
            functions.function_priority[function_in_use]->monitor_result;
          break;
        case acr_function_tcc_and_shared:
#endif
        case acr_function_shared_object_lib: // Better compilation finished
          pthread_spin_lock(&init_data->alternative_lock);
          init_data->alternative_function =
            functions.function_priority[function_in_use]->cc_function;
          init_data->alternative_still_usable =
            init_data->usability_inital_value;
          pthread_spin_unlock(&init_data->alternative_lock);
          init_data->current_monitoring_data =
            functions.function_priority[function_in_use]->monitor_result;
          break;
        case acr_function_empty: // Cloog has not finished
          break;
      }
    } else {
      // Function no more suitable
      pthread_spin_lock(&init_data->alternative_lock);
      init_data->alternative_still_usable = 0;
      pthread_spin_unlock(&init_data->alternative_lock);
      init_data->current_monitoring_data = NULL;

      // Round robin function
      function_in_use = (function_in_use + 1) % num_functions;

      pthread_mutex_lock(&cloog_thread_data.mutex);
      while(cloog_thread_data.num_threads ==
          cloog_thread_data.num_threads_compiling) { // Previous CLooG not finished
        pthread_cond_wait(&cloog_thread_data.coordinator_sleep,
            &cloog_thread_data.mutex);
      }
      // CLooG with last valid result
      pthread_spin_lock(&monitor_data.spinlock);
      if (monitor_data.current_valid_computation != NULL) { // Most recent data
        monitor_data.scrap_values = valid_monitor_result;
        valid_monitor_result = monitor_data.current_valid_computation;
        monitor_data.current_valid_computation = NULL;
      }
      pthread_spin_unlock(&monitor_data.spinlock);

      cloog_thread_data.where_to_add =
        functions.function_priority[function_in_use];
      cloog_thread_data.monitor_result = valid_monitor_result;
      cloog_thread_data.generate_function = true;
      pthread_cond_signal(&cloog_thread_data.compiler_thread_sleep);
      pthread_mutex_unlock(&cloog_thread_data.mutex);
      size_t considered_old_function = function_in_use == 0 ?
        num_functions - 1 : function_in_use - 1;
      do {
        pthread_spin_lock(&functions.function_priority[function_in_use]->lock);
        type = functions.function_priority[function_in_use]->type;
        pthread_spin_unlock(&functions.function_priority[function_in_use]->lock);
        if (type == acr_function_finished_cloog_gen) {
          break;
        }
        else {
          unsigned char* monitor_result;
          pthread_spin_lock(&functions.function_priority[considered_old_function]->lock);
          type = functions.function_priority[considered_old_function]->type;
          monitor_result = functions.function_priority[considered_old_function]->monitor_result;
          pthread_spin_unlock(&functions.function_priority[considered_old_function]->lock);
          switch (type) {
            case acr_function_shared_object_lib:
#ifdef TCC_PRESENT
            case acr_function_tcc_and_shared:
#endif
              if (acr_verify_me(init_data->monitor_total_size, monitor_result,
                    valid_monitor_result)) { // Old one valid
                fprintf(stderr, "old ok\n");
                pthread_spin_lock(&init_data->alternative_lock);
#ifdef TCC_PRESENT
                if (type == acr_function_tcc_in_memory) {
                  init_data->alternative_function =
                    functions.function_priority[considered_old_function]->tcc_function;
                } else
#endif
                {
                  init_data->alternative_function =
                    functions.function_priority[considered_old_function]->cc_function;
                }
                init_data->alternative_still_usable =
                  init_data->usability_inital_value;
                pthread_spin_unlock(&init_data->alternative_lock);
                init_data->current_monitoring_data =
                  functions.function_priority[function_in_use]->monitor_result;
                function_in_use = (function_in_use + 1) % num_functions;
                struct func_value *temp = functions.function_priority[considered_old_function];
                functions.function_priority[considered_old_function] =
                  functions.function_priority[function_in_use];
                functions.function_priority[function_in_use] = temp;
              }
              break;
#ifdef TCC_PRESENT
            case acr_function_tcc_in_memory:
              break;
#endif
            default:
              break;
          }
        }
        considered_old_function = considered_old_function == 0 ?
          num_functions - 1 : considered_old_function - 1;
      } while (considered_old_function != function_in_use);
    }
  }

  // Wait for compile thread to finish
  pthread_mutex_lock(&compile_threads_data.mutex);
  compile_threads_data.end_yourself = true;
  pthread_mutex_unlock(&compile_threads_data.mutex);
  pthread_cond_broadcast(&compile_threads_data.compiler_thread_sleep);

  pthread_mutex_lock(&cloog_thread_data.mutex);
  cloog_thread_data.end_yourself = true;
  pthread_mutex_unlock(&cloog_thread_data.mutex);
  pthread_cond_broadcast(&cloog_thread_data.compiler_thread_sleep);
  monitor_data.end_yourself = true;

  for (size_t i = 0; i < num_compilation_threads; ++i) {
    pthread_join(compile_threads[i], NULL);
  }
  pthread_mutex_destroy(&compile_threads_data.mutex);
  pthread_cond_destroy(&compile_threads_data.compiler_thread_sleep);
  pthread_cond_destroy(&compile_threads_data.coordinator_sleep);
  free(compile_threads);

  for (size_t i = 0; i < num_cloog_threads; ++i) {
    pthread_join(cloog_threads[i], NULL);
  }
  pthread_mutex_destroy(&cloog_thread_data.mutex);
  pthread_cond_destroy(&cloog_thread_data.compiler_thread_sleep);
  pthread_cond_destroy(&cloog_thread_data.coordinator_sleep);
  pthread_join(monitoring_thread, NULL);
  pthread_spin_destroy(&monitor_data.spinlock);
  free(cloog_threads);

  // Clean all
  for (size_t i = 0; i < functions.total_functions; ++i) {
    free(functions.value[i].monitor_result);
    pthread_spin_destroy(&functions.value[i].lock);
    switch(functions.value[i].type) {
      case acr_function_shared_object_lib:
        dlclose(functions.value[i].
            compiler_specific.shared_obj_lib.dlhandle);
        break;
#ifdef TCC_PRESENT
      case acr_function_tcc_in_memory:
        break;
      case acr_function_tcc_and_shared:
        dlclose(functions.value[i].
            compiler_specific.shared_obj_lib.dlhandle);
        tcc_delete(functions.value[i].compiler_specific.tcc.state);
        break;
#endif
      case acr_function_finished_cloog_gen:
      case acr_function_started_compilation:
      case acr_function_empty:
        break;
    }
  }
  free(functions.value);
  free(functions.function_priority);
  free(valid_monitor_result);
  pthread_exit(NULL);
}

void* acr_cloog_generate_code_from_alt(void* in_data) {
  struct acr_runtime_threads_cloog_gencode * input_data =
    (struct acr_runtime_threads_cloog_gencode *) in_data;

  const size_t monitor_total_size = input_data->monitor_total_size;
  struct acr_runtime_data *rdata = input_data->rdata;

  for (;;) {
    char* generated_code;
    size_t size_code;
    struct func_value *where_to_add;
    unsigned char *monitor_result;
    bool has_to_stop;

    pthread_mutex_lock(&input_data->mutex);


    if(input_data->num_threads_compiling == input_data->num_threads) {
      pthread_cond_signal(&input_data->coordinator_sleep);
    }
    input_data->num_threads_compiling--;
    while(!input_data->generate_function && !input_data->end_yourself) {
      pthread_cond_wait(&input_data->compiler_thread_sleep, &input_data->mutex);
    }
    input_data->generate_function = false;
    input_data->num_threads_compiling++;
    has_to_stop = input_data->end_yourself;
    monitor_result = input_data->monitor_result;
    where_to_add = input_data->where_to_add;

    pthread_mutex_unlock(&input_data->mutex);

    if (has_to_stop)
      break;

    FILE* new_code = open_memstream(&generated_code, &size_code);
    fprintf(new_code, //"#include \"acr_required_definitions.h\"\n"
        "void acr_alternative_function%s {\n",
        rdata->function_prototype);
    acr_cloog_generate_alternative_code_from_input(new_code, rdata,
        monitor_result);
    fprintf(new_code, "}\n");
    fclose(new_code);

    memcpy(where_to_add->monitor_result, monitor_result,
        monitor_total_size);
    pthread_spin_lock(&where_to_add->lock);
    where_to_add->type =
      acr_function_finished_cloog_gen;
    where_to_add->generated_code =
      generated_code;
    pthread_spin_unlock(&where_to_add->lock);
  }
  pthread_exit(NULL);
}

#ifdef TCC_PRESENT
static void* acr_runtime_compile_tcc(void* in_data) {
  struct acr_runtime_threads_compile_tcc *input_data =
    (struct acr_runtime_threads_compile_tcc *) in_data;

  struct func_value *where_to_add;

  bool has_to_stop;
  for (;;) {

    pthread_mutex_lock(&input_data->mutex);
    input_data->compile_something = false;
    pthread_cond_signal(&input_data->waking_up);
    while(!input_data->compile_something && !input_data->end_yourself) {
      pthread_cond_wait(&input_data->waking_up, &input_data->mutex);
    }
    has_to_stop = input_data->end_yourself;
    where_to_add = input_data->where_to_add;
    pthread_mutex_unlock(&input_data->mutex);

    if (has_to_stop)
      break;

    TCCState *tccstate =
      acr_compile_with_tcc(input_data->generated_code);
    void *function =
      tcc_get_symbol(tccstate, "acr_alternative_function");

    pthread_spin_lock(&where_to_add->lock);

    where_to_add->compiler_specific.tcc.state =
      tccstate;
    where_to_add->tcc_function = function;
    where_to_add->type =
     where_to_add->type == acr_function_shared_object_lib ?
     acr_function_tcc_and_shared : acr_function_tcc_in_memory;
    where_to_add->type +=
      acr_function_tcc_in_memory;

    pthread_spin_unlock(&where_to_add->lock);
  }
  pthread_exit(NULL);
}
#endif

void* acr_runtime_compile_thread(void* in_data) {
  struct acr_runtime_threads_compile_data * input_data =
    (struct acr_runtime_threads_compile_data *) in_data;

  pthread_t tcc_thread;
  char* file;
  struct func_value *where_to_add;

#ifdef TCC_PRESENT
  struct acr_runtime_threads_compile_tcc tcc_data;
  tcc_data.compile_something = true;
  tcc_data.end_yourself = false;
  pthread_mutex_init(&tcc_data.mutex, NULL);
  pthread_cond_init(&tcc_data.waking_up, NULL);
  pthread_create(&tcc_thread, NULL, acr_runtime_compile_tcc, (void*)&tcc_data);
#endif

  for (;;) {
    char *code_to_compile;
    bool has_to_stop;

    pthread_mutex_lock(&input_data->mutex);

    if(input_data->num_threads_compiling == input_data->num_threads) {
      pthread_cond_signal(&input_data->coordinator_sleep);
    }
    input_data->num_threads_compiling--;

    while(!input_data->compile_something && !input_data->end_yourself) {
      pthread_cond_wait(&input_data->compiler_thread_sleep, &input_data->mutex);
    }
    input_data->num_threads_compiling++;
    input_data->compile_something = false;
    has_to_stop = input_data->end_yourself;
    where_to_add = input_data->where_to_add;
    pthread_mutex_unlock(&input_data->mutex);

    if (has_to_stop)
      break;

    code_to_compile = where_to_add->generated_code;

#ifdef TCC_PRESENT
    tcc_data.generated_code = code_to_compile;
    tcc_data.where_to_add = where_to_add;
    pthread_mutex_lock(&tcc_data.mutex);
    while (tcc_data.compile_something == true) {
      pthread_cond_wait(&tcc_data.waking_up, &tcc_data.mutex);
    }
    tcc_data.compile_something = true;
    pthread_cond_signal(&tcc_data.waking_up);
    pthread_mutex_unlock(&tcc_data.mutex);
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

    pthread_spin_lock(&where_to_add->lock);
    where_to_add->
      compiler_specific.shared_obj_lib.dlhandle = dlhandle;
    where_to_add->cc_function = function;
    where_to_add->type =
#ifdef TCC_PRESENT
     where_to_add->type == acr_function_tcc_in_memory ?
     acr_function_tcc_and_shared : acr_function_shared_object_lib;
#else
     acr_function_shared_object_lib;
#endif
    pthread_spin_unlock(&where_to_add->lock);
    if(unlink(file) == -1) {
      perror("unlink");
      exit(EXIT_FAILURE);
    }
    free(file);
  }
#ifdef TCC_PRESENT
  pthread_mutex_lock(&tcc_data.mutex);
  tcc_data.end_yourself = true;
  pthread_cond_signal(&tcc_data.waking_up);
  pthread_mutex_unlock(&tcc_data.mutex);
  pthread_join(tcc_thread, NULL);
  pthread_mutex_destroy(&tcc_data.mutex);
  pthread_cond_destroy(&tcc_data.waking_up);
#endif

  pthread_exit(NULL);
}
