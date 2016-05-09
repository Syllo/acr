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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "acr/acr_runtime_build.h"
#include "acr/acr_runtime_data.h"
#include "acr/acr_runtime_verify.h"
#include "acr/acr_stats.h"
#include "acr/cloog_runtime.h"

static void* acr_runtime_monitoring_function(void* in_data);

static void* acr_runtime_compile_thread(void* in_data);

static void* acr_cloog_generate_code_from_alt(void* in_data);

enum acr_avaliable_function_type {
  acr_function_empty,
  acr_function_started_cloog_gen,
  acr_function_finished_cloog_gen,
  acr_function_started_compilation,
  acr_function_shared_object_lib,
#ifdef TCC_PRESENT
  acr_function_tcc_in_memory,
  acr_function_tcc_and_shared,
#endif
};

struct acr_avaliable_functions {
  size_t total_functions;
  struct func_value{
    pthread_spinlock_t lock;
#ifdef TCC_PRESENT
    void *tcc_function;
#endif
    void *cc_function;
    unsigned char *monitor_result;
    FILE *memstream;
    size_t sizeof_string;
    char *generated_code;
    struct {
      struct {
        void *dlhandle;
      } shared_obj_lib;
#ifdef TCC_PRESENT
      struct {
        TCCState *state;
      } tcc;
#endif
    } compiler_specific;
    enum acr_avaliable_function_type type;
  } *value;
  struct func_value **function_priority;
};

struct acr_monitoring_computation {
  void (*monitoring_function)(unsigned char*);
  unsigned char *current_valid_computation;
  unsigned char *scrap_values;
  size_t monitor_result_size;
#ifdef ACR_STATS_ENABLED
  size_t num_mesurement;
  double total_time;
#endif
  pthread_spinlock_t spinlock;
  bool end_yourself;
};

struct acr_runtime_threads_cloog_gencode {
  size_t num_threads;
  size_t num_threads_compiling;
  struct func_value *where_to_add;
  struct acr_runtime_data *rdata;
#ifdef ACR_STATS_ENABLED
  size_t num_mesurement;
  double total_time;
#endif
  pthread_mutex_t mutex;
  pthread_cond_t compiler_thread_sleep;
  pthread_cond_t coordinator_sleep;
  bool generate_function;
  bool end_yourself;
};

struct acr_runtime_threads_compile_data {
  size_t num_threads;
  size_t num_threads_compiling;
  struct func_value *where_to_add;
  size_t num_cflags;
  char **cflags;
#ifdef ACR_STATS_ENABLED
  size_t num_mesurement;
  double total_time;
  size_t num_tcc_mesurement;
  double total_tcc_time;
#endif
  pthread_mutex_t mutex;
  pthread_cond_t compiler_thread_sleep;
  pthread_cond_t coordinator_sleep;
  bool compile_something;
  bool end_yourself;
};

#ifdef TCC_PRESENT
struct acr_runtime_threads_compile_tcc {
  struct func_value *where_to_add;
#ifdef ACR_STATS_ENABLED
  size_t num_mesurement;
  double total_time;
#endif
  pthread_mutex_t mutex;
  pthread_cond_t waking_up;
  bool compile_something;
  bool end_yourself;
};
#endif

static void* acr_runtime_monitoring_function(void *in_data) {
  struct acr_monitoring_computation *input_data =
    (struct acr_monitoring_computation*) in_data;

#ifdef ACR_STATS_ENABLED
  double total_time = 0.;
  size_t num_mesurement = 0;
#endif

  unsigned char *monitor_result =
    malloc(input_data->monitor_result_size * sizeof(*monitor_result));

  void (*const monitoring_function)(unsigned char*) =
    input_data->monitoring_function;

  bool end_myself;
  while(1) {
#ifdef ACR_STATS_ENABLED
    acr_time tstart;
    acr_get_current_time(&tstart);
#endif
    monitoring_function(monitor_result);
#ifdef ACR_STATS_ENABLED
    acr_time tend;
    acr_get_current_time(&tend);
    total_time += acr_difftime(tstart, tend);
    num_mesurement += 1;
#endif
    pthread_spin_lock(&input_data->spinlock);
    if (input_data->current_valid_computation == NULL) {
      input_data->current_valid_computation = monitor_result;
      monitor_result = input_data->scrap_values;
    } else {
      input_data->scrap_values = input_data->current_valid_computation;;
      input_data->current_valid_computation = monitor_result;
      monitor_result = input_data->scrap_values;
    }
    end_myself = input_data->end_yourself;
    pthread_spin_unlock(&input_data->spinlock);
    if (end_myself)
      break;
  }

  free(monitor_result);
  if (input_data->current_valid_computation) {
    free(input_data->current_valid_computation);
  } else {
    free(input_data->scrap_values);
  }

#ifdef ACR_STATS_ENABLED
  input_data->total_time = total_time;
  input_data->num_mesurement = num_mesurement;
#endif

  pthread_exit(NULL);
}

void* acr_verification_and_coordinator_function(void *in_data) {
  struct acr_runtime_data *init_data =
    (struct acr_runtime_data*) in_data;

  const size_t num_functions = 2;
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
    functions.value[i].memstream =
      open_memstream(&functions.value[i].generated_code,
          &functions.value[i].sizeof_string);
  }

  // Monitoring thread
  pthread_t monitoring_thread;
  struct acr_monitoring_computation monitor_data = {
    .monitoring_function = init_data->monitoring_function,
    .current_valid_computation = NULL,
    .monitor_result_size = init_data->monitor_total_size,
    .end_yourself = false,
#ifdef ACR_STATS_ENABLED
    .num_mesurement = 0,
    .total_time = 0.,
#endif
  };
  pthread_spin_init(&monitor_data.spinlock, PTHREAD_PROCESS_PRIVATE);
  monitor_data.scrap_values =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data.scrap_values));
  pthread_create(&monitoring_thread, NULL, acr_runtime_monitoring_function,
      (void*) &monitor_data);

  // Compile threads
  const size_t num_compilation_threads = num_functions;
  struct acr_runtime_threads_compile_data compile_threads_data = {
    .cflags = init_data->compiler_flags,
    .num_cflags = init_data->num_compiler_flags,
    .end_yourself = false,
    .compile_something = false,
    .num_threads = num_compilation_threads,
    .num_threads_compiling = num_compilation_threads,
#ifdef ACR_STATS_ENABLED
    .num_mesurement = 0,
    .total_time = 0.,
    .num_tcc_mesurement = 0,
    .total_tcc_time = 0.,
#endif
  };

  pthread_t *compile_threads =
    malloc(num_compilation_threads * sizeof(*compile_threads));
  pthread_mutex_init(&compile_threads_data.mutex, NULL);
  pthread_cond_init(&compile_threads_data.compiler_thread_sleep, NULL);
  pthread_cond_init(&compile_threads_data.coordinator_sleep, NULL);
  for (size_t i = 0; i < num_compilation_threads; ++i) {
    pthread_create(&compile_threads[i], NULL,
        acr_runtime_compile_thread, (void*)&compile_threads_data);
  }

  // Cloog thread
  const size_t num_cloog_threads = 1; // We need to fix that
  pthread_t *cloog_threads =
    malloc(num_cloog_threads * sizeof(*compile_threads));
  struct acr_runtime_threads_cloog_gencode cloog_thread_data = {
    .end_yourself = false,
    .generate_function = false,
    .num_threads = num_cloog_threads,
    .num_threads_compiling = num_cloog_threads,
    .rdata = init_data,
#ifdef ACR_STATS_ENABLED
    .num_mesurement = 0,
    .total_time = 0.,
#endif
  };
  pthread_mutex_init(&cloog_thread_data.mutex, NULL);
  pthread_cond_init(&cloog_thread_data.coordinator_sleep, NULL);
  pthread_cond_init(&cloog_thread_data.compiler_thread_sleep, NULL);
  for (size_t i = 0; i < num_cloog_threads; ++i) {
    pthread_create(&cloog_threads[i], NULL, acr_cloog_generate_code_from_alt,
        (void*)&cloog_thread_data);
  }

  unsigned char *valid_monitor_result = NULL;
  unsigned char *invalid_monitor_result =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data.scrap_values));
  size_t function_in_use = 0;

  // Start gencode

  // Wait for one valid monitor data
  while (valid_monitor_result == NULL) {
    pthread_spin_lock(&monitor_data.spinlock);
    if (monitor_data.current_valid_computation) {
      valid_monitor_result = monitor_data.current_valid_computation;
      monitor_data.current_valid_computation = NULL;
      monitor_data.scrap_values = invalid_monitor_result;
    }
    pthread_spin_unlock(&monitor_data.spinlock);
  }
  invalid_monitor_result = NULL;

  // Gen with CLooG the first valid function
  pthread_mutex_lock(&cloog_thread_data.mutex);
  while(cloog_thread_data.num_threads == cloog_thread_data.num_threads_compiling) {
    pthread_cond_wait(&cloog_thread_data.coordinator_sleep, &cloog_thread_data.mutex);
  }
  cloog_thread_data.where_to_add = functions.function_priority[function_in_use];
  invalid_monitor_result = cloog_thread_data.where_to_add->monitor_result;
  cloog_thread_data.where_to_add->monitor_result = valid_monitor_result;
  cloog_thread_data.generate_function = true;
  functions.function_priority[function_in_use]->type =
    acr_function_started_cloog_gen;
  pthread_mutex_unlock(&cloog_thread_data.mutex);
  valid_monitor_result = NULL;

  // CLooG it's your time to shine
  pthread_cond_signal(&cloog_thread_data.compiler_thread_sleep);

  while (init_data->monitor_thread_continue) {

    // Get last valid monitor result
    while (valid_monitor_result == NULL) {
      pthread_spin_lock(&monitor_data.spinlock);
      if (monitor_data.current_valid_computation) {
        valid_monitor_result = monitor_data.current_valid_computation;
        monitor_data.current_valid_computation = NULL;
        monitor_data.scrap_values = invalid_monitor_result;
      }
      pthread_spin_unlock(&monitor_data.spinlock);
    }
    invalid_monitor_result = NULL;

    enum acr_avaliable_function_type type;
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
        case acr_function_started_cloog_gen: // Cloog has not finished
          break;
        case acr_function_empty: // Cloog has not finished
          fprintf(stderr, "Empty function in test\n");
          exit(1);
          break;
      }
      invalid_monitor_result = valid_monitor_result;
      valid_monitor_result = NULL;
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
          cloog_thread_data.num_threads_compiling) { // All CLooGs are busy
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
      invalid_monitor_result = cloog_thread_data.where_to_add->monitor_result;
      cloog_thread_data.where_to_add->monitor_result = valid_monitor_result;
      cloog_thread_data.generate_function = true;
      functions.function_priority[function_in_use]->type =
        acr_function_started_cloog_gen;
      pthread_mutex_unlock(&cloog_thread_data.mutex);
      pthread_cond_signal(&cloog_thread_data.compiler_thread_sleep);
      valid_monitor_result = NULL;
    }
  }

  // Quit compile threads
  pthread_mutex_lock(&compile_threads_data.mutex);
  compile_threads_data.end_yourself = true;
  pthread_mutex_unlock(&compile_threads_data.mutex);
  pthread_cond_broadcast(&compile_threads_data.compiler_thread_sleep);
  for (size_t i = 0; i < num_compilation_threads; ++i) {
    pthread_join(compile_threads[i], NULL);
  }
  pthread_mutex_destroy(&compile_threads_data.mutex);
  pthread_cond_destroy(&compile_threads_data.compiler_thread_sleep);
  pthread_cond_destroy(&compile_threads_data.coordinator_sleep);
  free(compile_threads);

  // Quit cloog threads
  pthread_mutex_lock(&cloog_thread_data.mutex);
  cloog_thread_data.end_yourself = true;
  pthread_mutex_unlock(&cloog_thread_data.mutex);
  pthread_cond_broadcast(&cloog_thread_data.compiler_thread_sleep);
  for (size_t i = 0; i < num_cloog_threads; ++i) {
    pthread_join(cloog_threads[i], NULL);
  }
  pthread_mutex_destroy(&cloog_thread_data.mutex);
  pthread_cond_destroy(&cloog_thread_data.compiler_thread_sleep);
  pthread_cond_destroy(&cloog_thread_data.coordinator_sleep);

  // Quit monitoring thread
  monitor_data.end_yourself = true;
  pthread_join(monitoring_thread, NULL);
  pthread_spin_destroy(&monitor_data.spinlock);
  free(cloog_threads);

  // Clean all
  for (size_t i = 0; i < functions.total_functions; ++i) {
    fclose(functions.value[i].memstream);
    free(functions.value[i].generated_code);
    free(functions.value[i].monitor_result);
    pthread_spin_destroy(&functions.value[i].lock);
    switch(functions.value[i].type) {
#ifdef TCC_PRESENT
      case acr_function_tcc_in_memory:
        tcc_delete(functions.value[i].compiler_specific.tcc.state);
        break;
      case acr_function_tcc_and_shared:
        tcc_delete(functions.value[i].compiler_specific.tcc.state);
#endif
      case acr_function_shared_object_lib:
        dlclose(functions.value[i].
            compiler_specific.shared_obj_lib.dlhandle);
        break;
      case acr_function_finished_cloog_gen:
      case acr_function_started_compilation:
      case acr_function_started_cloog_gen:
      case acr_function_empty:
        break;
    }
  }

#ifdef ACR_STATS_ENABLED
    init_data->thread_stats.num_mesurements[acr_thread_time_cloog] =
      cloog_thread_data.num_mesurement;
    init_data->thread_stats.total_time[acr_thread_time_cloog] =
      cloog_thread_data.total_time;
    init_data->thread_stats.num_mesurements[acr_thread_time_monitor] =
      monitor_data.num_mesurement;
    init_data->thread_stats.total_time[acr_thread_time_monitor] =
      monitor_data.total_time;
    init_data->thread_stats.num_mesurements[acr_thread_time_cc] =
      compile_threads_data.num_mesurement;
    init_data->thread_stats.total_time[acr_thread_time_cc] =
      compile_threads_data.total_time;
    init_data->thread_stats.num_mesurements[acr_thread_time_tcc] =
      compile_threads_data.num_tcc_mesurement;
    init_data->thread_stats.total_time[acr_thread_time_tcc] =
      compile_threads_data.total_tcc_time;
#endif

  free(functions.value);
  free(functions.function_priority);
  free(valid_monitor_result);
  free(invalid_monitor_result);
  pthread_exit(NULL);
}

static void* acr_cloog_generate_code_from_alt(void* in_data) {
  struct acr_runtime_threads_cloog_gencode * input_data =
    (struct acr_runtime_threads_cloog_gencode *) in_data;

#ifdef ACR_STATS_ENABLED
  double total_time = 0.;
  size_t num_mesurement = 0;
#endif

  struct acr_runtime_data *rdata = input_data->rdata;
  FILE* stream;

  for (;;) {
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
    where_to_add = input_data->where_to_add;

    pthread_mutex_unlock(&input_data->mutex);

    if (has_to_stop)
      break;

    monitor_result = where_to_add->monitor_result;
    stream = where_to_add->memstream;
#ifdef ACR_STATS_ENABLED
    acr_time tstart;
    acr_get_current_time(&tstart);
#endif

    fseek(stream, 0l, SEEK_SET);
    fprintf(stream, //"#include \"acr_required_definitions.h\"\n"
        "void acr_alternative_function%s {\n",
        rdata->function_prototype);
    acr_cloog_generate_alternative_code_from_input(stream, rdata,
        monitor_result);
    fprintf(stream, "}\n");

    // Now the pointers in function structure are up to date
    fflush(stream);

    pthread_spin_lock(&where_to_add->lock);
    where_to_add->type = acr_function_finished_cloog_gen;
    pthread_spin_unlock(&where_to_add->lock);

#ifdef ACR_STATS_ENABLED
    acr_time tend;
    acr_get_current_time(&tend);
    total_time += acr_difftime(tstart, tend);
    num_mesurement += 1;
#endif
  }

#ifdef ACR_STATS_ENABLED
  pthread_mutex_lock(&input_data->mutex);
  input_data->total_time += total_time;
  input_data->num_mesurement += num_mesurement;
  pthread_mutex_unlock(&input_data->mutex);
#endif

  pthread_exit(NULL);
}

#ifdef TCC_PRESENT
static void* acr_runtime_compile_tcc(void* in_data) {
  struct acr_runtime_threads_compile_tcc *input_data =
    (struct acr_runtime_threads_compile_tcc *) in_data;

#ifdef ACR_STATS_ENABLED
  double total_time = 0.;
  size_t num_mesurement = 0;
#endif

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
    input_data->compile_something = true;
    pthread_mutex_unlock(&input_data->mutex);

    if (has_to_stop)
      break;

#ifdef ACR_STATS_ENABLED
    acr_time tstart;
    acr_get_current_time(&tstart);
#endif

    TCCState *tccstate =
      acr_compile_with_tcc(where_to_add->generated_code);
    void *function =
      tcc_get_symbol(tccstate, "acr_alternative_function");

    pthread_spin_lock(&where_to_add->lock);

    where_to_add->compiler_specific.tcc.state =
      tccstate;
    where_to_add->tcc_function = function;
    where_to_add->type =
     where_to_add->type == acr_function_shared_object_lib ?
     acr_function_tcc_and_shared : acr_function_tcc_in_memory;

    pthread_spin_unlock(&where_to_add->lock);

#ifdef ACR_STATS_ENABLED
    acr_time tend;
    acr_get_current_time(&tend);
    total_time += acr_difftime(tstart, tend);
    num_mesurement += 1;
#endif
  }

#ifdef ACR_STATS_ENABLED
  input_data->total_time = total_time;
  input_data->num_mesurement = num_mesurement;
#endif

  pthread_exit(NULL);
}
#endif

static void* acr_runtime_compile_thread(void* in_data) {
  struct acr_runtime_threads_compile_data * input_data =
    (struct acr_runtime_threads_compile_data *) in_data;

#ifdef ACR_STATS_ENABLED
  double total_time = 0.;
  size_t num_mesurement = 0;
#endif

  char* file;
  struct func_value *where_to_add;

#ifdef TCC_PRESENT
  pthread_t tcc_thread;
  struct acr_runtime_threads_compile_tcc tcc_data;
  tcc_data.compile_something = true;
  tcc_data.end_yourself = false;
  pthread_mutex_init(&tcc_data.mutex, NULL);
  pthread_cond_init(&tcc_data.waking_up, NULL);
  pthread_create(&tcc_thread, NULL, acr_runtime_compile_tcc, (void*)&tcc_data);
#endif

  for (;;) {
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

#ifdef ACR_STATS_ENABLED
    acr_time tstart;
    acr_get_current_time(&tstart);
#endif

#ifdef TCC_PRESENT
    pthread_mutex_lock(&tcc_data.mutex);
    tcc_data.where_to_add = where_to_add;
    while (tcc_data.compile_something == true) {
      pthread_cond_wait(&tcc_data.waking_up, &tcc_data.mutex);
    }
    tcc_data.compile_something = true;
    pthread_cond_signal(&tcc_data.waking_up);
    pthread_mutex_unlock(&tcc_data.mutex);
#endif
    file =
      acr_compile_with_system_compiler(where_to_add->generated_code,
          input_data->num_cflags,
          input_data->cflags, NULL);
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

#ifdef ACR_STATS_ENABLED
    acr_time tend;
    acr_get_current_time(&tend);
    total_time += acr_difftime(tstart, tend);
    num_mesurement += 1;
#endif
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

#ifdef ACR_STATS_ENABLED
  pthread_mutex_lock(&input_data->mutex);
  input_data->total_time += total_time;
  input_data->num_mesurement += num_mesurement;
#ifdef TCC_PRESENT
  input_data->total_tcc_time += tcc_data.total_time;
  input_data->num_tcc_mesurement += tcc_data.num_mesurement;
#endif
  pthread_mutex_unlock(&input_data->mutex);
#endif

  pthread_exit(NULL);
}
