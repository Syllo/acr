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
#include <time.h>
#include <stdatomic.h>

#include "acr/acr_runtime_build.h"
#include "acr/acr_runtime_code_generation.h"
#include "acr/acr_runtime_data.h"
#include "acr/acr_runtime_perf.h"
#include "acr/acr_runtime_verify.h"
#include "acr/acr_stats.h"

static void* acr_runtime_monitoring_function(void* in_data);

static void* acr_runtime_compile_thread(void* in_data);

static void* acr_cloog_generate_code_from_alt(void* in_data);

enum acr_avaliable_function_type {
  acr_function_empty,
  acr_function_proposed_cloog_gen,
  acr_function_started_cloog_gen,
  acr_function_finished_cloog_gen,
  acr_function_proposed_compilation,
  acr_function_shared_object_lib,
#ifdef TCC_PRESENT
  acr_function_tcc_in_memory,
  acr_function_tcc_and_shared,
#endif
};

enum acr_kernel_function_type {
  acr_kernel_function_initial,
#ifdef TCC_PRESENT
  acr_kernel_function_proposed_tcc,
  acr_kernel_function_using_tcc,
#endif
  acr_kernel_function_proposed_cc,
  acr_kernel_function_using_cc,
};

struct acr_avaliable_functions {
  size_t total_functions;
  struct func_value {
#ifdef TCC_PRESENT
    void *tcc_function;
#endif
    void *cc_function;
    unsigned char *monitor_result;
    unsigned char *monitor_untouched;
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
    _Atomic enum acr_avaliable_function_type type;
  } *value;
  struct func_value **function_priority;
};

struct acr_monitoring_shared {
  _Atomic (unsigned char *) current_valid_computation;
  unsigned char *scrap_values;
};

struct acr_monitoring_computation {
  void (*monitoring_function)(unsigned char*);
  size_t monitor_result_size;
#ifdef ACR_STATS_ENABLED
  size_t num_mesurement;
  double total_time;
#endif
  struct acr_runtime_kernel_info *kernel_info;
  struct acr_monitoring_shared *shared_buffer;
  atomic_flag end_yourself;
  pthread_cond_t *sleep_cond;
  pthread_cond_t *coordinator_continue_cond;
};

struct acr_runtime_threads_cloog_gencode {
  size_t thread_num;
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
  pthread_cond_t *coordinator_continue_cond;
  bool generate_function;
  volatile bool end_yourself;
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
  pthread_cond_t *coordinator_continue_cond;
  bool compile_something;
  volatile bool end_yourself;
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
  pthread_cond_t *coordinator_continue_cond;
  bool compile_something;
  volatile bool end_yourself;
};
#endif

static void* acr_runtime_monitoring_function(void *in_data) {
  struct acr_monitoring_computation * const input_data =
    (struct acr_monitoring_computation*) in_data;

#ifdef ACR_STATS_ENABLED
  double total_time = 0.;
  size_t num_mesurement = 0;
#endif

  struct acr_runtime_kernel_info volatile* kernel_info = input_data->kernel_info;

  unsigned char *monitor_result =
    malloc(input_data->monitor_result_size * sizeof(*monitor_result));
  unsigned char *expected_value = monitor_result;

  void (*const monitoring_function)(unsigned char*) =
    input_data->monitoring_function;

  double last_time_updated, compute_time;
  size_t last_kernel_id = 0;
  acr_time t1, t2;
  acr_get_current_time(&t1);
  pthread_mutex_t mut;
  pthread_mutex_init(&mut, NULL);
  while(atomic_flag_test_and_set_explicit(&input_data->end_yourself, memory_order_relaxed)) {

    struct acr_runtime_kernel_info kinfo = *kernel_info;
    if (last_kernel_id != kinfo.num_calls) {
      last_kernel_id = kinfo.num_calls;

      acr_time tstart;
      acr_get_current_time(&tstart);

      monitoring_function(monitor_result);

      acr_get_current_time(&t1);
      compute_time = acr_difftime(tstart, t1);
#ifdef ACR_STATS_ENABLED
      total_time += compute_time;
      num_mesurement += 1;
#endif

      if (atomic_compare_exchange_strong_explicit(
          &input_data->shared_buffer->current_valid_computation,
          &expected_value,
          monitor_result,
          memory_order_release,
          memory_order_acquire)) { // The pointer was what we expexted (monitor swap)
        unsigned char *old = expected_value;
        expected_value = monitor_result;
        monitor_result = old;
      } else { // The coordinator took the last version
        unsigned char *old = input_data->shared_buffer->scrap_values;
        if (expected_value != NULL)
          fprintf(stderr, "Error in algorithm noob\n");
        atomic_store_explicit(
            &input_data->shared_buffer->current_valid_computation,
            monitor_result,
            memory_order_release);
        pthread_cond_signal(input_data->coordinator_continue_cond);
        expected_value = monitor_result;
        monitor_result = old;
      }
    } else {
      pthread_mutex_lock(&mut);
      pthread_cond_wait(input_data->sleep_cond, &mut);
      pthread_mutex_unlock(&mut);
    }
  }

  free(monitor_result);
  if (input_data->shared_buffer->current_valid_computation) {
    free(input_data->shared_buffer->current_valid_computation);
  } else {
    free(input_data->shared_buffer->scrap_values);
  }

#ifdef ACR_STATS_ENABLED
  input_data->total_time = total_time;
  input_data->num_mesurement = num_mesurement;
#endif

  pthread_exit(NULL);
}

static void acr_valid_function_switch_to(enum acr_avaliable_function_type type,
    enum acr_kernel_function_type *function_used_by_kernel_type,
    size_t *restrict function_used_by_kernel,
    size_t *restrict function_proposed_to_kernel,
    const size_t most_recent_function,
    struct acr_runtime_data *const init_data,
    struct acr_avaliable_functions *const functions,
    struct acr_runtime_threads_compile_data *const compile_threads_data) {

  switch(type) {
    case acr_function_finished_cloog_gen:  // missing C compilation
      /*fprintf(stderr, "Compiling %zu\n", most_recent_function);*/
      pthread_mutex_lock(&compile_threads_data->mutex);
      while((compile_threads_data->compile_something &&
            compile_threads_data->where_to_add !=
            functions->function_priority[most_recent_function]) ||
          compile_threads_data->num_threads ==
          compile_threads_data->num_threads_compiling) {
        pthread_cond_wait(&compile_threads_data->coordinator_sleep,
            &compile_threads_data->mutex);
      }
      atomic_store_explicit(
          &functions->function_priority[most_recent_function]->type,
          acr_function_proposed_compilation,
          memory_order_relaxed);
      compile_threads_data->compile_something = true;
      compile_threads_data->where_to_add =
        functions->function_priority[most_recent_function];
      pthread_cond_signal(&compile_threads_data->compiler_thread_sleep);
      pthread_mutex_unlock(&compile_threads_data->mutex);
      break;
    case acr_function_proposed_compilation: // - Waiting compilation
      break;
#ifdef TCC_PRESENT
    case acr_function_tcc_in_memory: // Fast compilation finished
      if (*function_used_by_kernel_type == acr_kernel_function_initial) {
        /*fprintf(stderr, "Propose tcc %zu\n", most_recent_function);*/
        atomic_store_explicit(&init_data->alternative_function,
            functions->function_priority[most_recent_function]->tcc_function,
            memory_order_relaxed);
        atomic_store_explicit(&init_data->current_monitoring_data,
          functions->function_priority[most_recent_function]->monitor_result,
          memory_order_relaxed);
        *function_proposed_to_kernel = most_recent_function;
        *function_used_by_kernel_type = acr_kernel_function_proposed_tcc;
      } else {
        if (*function_used_by_kernel_type == acr_kernel_function_proposed_tcc) {
          void *function_pointer = atomic_load_explicit(
              &init_data->alternative_function,
              memory_order_relaxed);
          if (function_pointer == NULL) {
            /*fprintf(stderr, "Kernel use tcc %zu\n", most_recent_function);*/
            *function_used_by_kernel = *function_proposed_to_kernel;
            *function_used_by_kernel_type = acr_kernel_function_using_tcc;
          }
        }
      }
      break;
    case acr_function_tcc_and_shared:
    case acr_function_shared_object_lib: // Better compilation finished
      if (*function_used_by_kernel_type != acr_kernel_function_proposed_cc) {
#else
    case acr_function_shared_object_lib: // Better compilation finished
      if (*function_used_by_kernel_type == acr_kernel_function_initial) {
#endif
        if (*function_used_by_kernel_type != acr_kernel_function_using_cc) {

          void *function_pointer = atomic_exchange_explicit(
              &init_data->alternative_function,
              functions->function_priority[most_recent_function]->cc_function,
              memory_order_relaxed);
          atomic_store_explicit(&init_data->current_monitoring_data,
              functions->function_priority[most_recent_function]->monitor_result,
              memory_order_relaxed);
#ifdef TCC_PRESENT
          if (*function_used_by_kernel_type == acr_kernel_function_proposed_tcc
              && function_pointer == NULL) {
            /*fprintf(stderr, "Kernel use tcc %zu\n", most_recent_function);*/
            *function_used_by_kernel = *function_proposed_to_kernel;
          }
#endif
          *function_used_by_kernel_type = acr_kernel_function_proposed_cc;
          *function_proposed_to_kernel = most_recent_function;
          /*fprintf(stderr, "Propose cc %zu\n", most_recent_function);*/
        }
      } else {
        void *function_pointer = atomic_load_explicit(
            &init_data->alternative_function,
            memory_order_relaxed);
        if (function_pointer == NULL) {
          *function_used_by_kernel = *function_proposed_to_kernel;
          /*fprintf(stderr, "Using kernel CC %zu\n", *function_used_by_kernel);*/
          *function_used_by_kernel_type = acr_kernel_function_using_cc;
        }
      }
      break;
    case acr_function_started_cloog_gen: // Cloog has not finished
    case acr_function_proposed_cloog_gen:
      break;
    case acr_function_empty:
      fprintf(stderr, "Empty function in test\n");
      exit(1);
      break;
  }
}

static void acr_get_most_recent_monitor_result(
    unsigned char **const restrict valid_monitor_result,
    unsigned char **const restrict invalid_monitor_result,
    struct acr_monitoring_computation *const monitor_data) {
  unsigned char *exch = atomic_load_explicit(
      &monitor_data->shared_buffer->current_valid_computation,
      memory_order_relaxed);
  if (exch) {
    monitor_data->shared_buffer->scrap_values = *invalid_monitor_result;
    *invalid_monitor_result = NULL;
    *valid_monitor_result = atomic_exchange_explicit(
        &monitor_data->shared_buffer->current_valid_computation,
        NULL,
        memory_order_acq_rel);
  }
}

static void acr_cloog_compilation(
    unsigned char ** restrict valid_monitor_result,
    unsigned char ** restrict invalid_monitor_result,
    size_t most_recent_function,
    struct acr_avaliable_functions *const functions,
    struct acr_runtime_threads_cloog_gencode *const cloog_thread_data) {

  while(cloog_thread_data->num_threads == cloog_thread_data->num_threads_compiling) {
    pthread_cond_wait(&cloog_thread_data->coordinator_sleep, &cloog_thread_data->mutex);
  }
  cloog_thread_data->where_to_add =
    functions->function_priority[most_recent_function];
  *invalid_monitor_result = cloog_thread_data->where_to_add->monitor_result;
  cloog_thread_data->where_to_add->monitor_result = *valid_monitor_result;
  cloog_thread_data->generate_function = true;
  atomic_store_explicit(
      &functions->function_priority[most_recent_function]->type,
      acr_function_proposed_cloog_gen,
      memory_order_relaxed);
  pthread_mutex_unlock(&cloog_thread_data->mutex);
  *valid_monitor_result = NULL;

  // CLooG it's your time to shine
  pthread_cond_signal(&cloog_thread_data->compiler_thread_sleep);
}

static inline void discard_kernel_function(
    struct acr_runtime_data *const init_data,
    enum acr_kernel_function_type *function_used_by_kernel_type) {

  atomic_store_explicit(
      &init_data->alternative_function,
      init_data->original_function,
      memory_order_relaxed);
  atomic_store_explicit(
      &init_data->current_monitoring_data,
      NULL,
      memory_order_relaxed);
  *function_used_by_kernel_type = acr_kernel_function_initial;
}

static inline size_t acr_next_free_function_position(
    size_t most_recent_function,
    size_t function_used_by_kernel,
    size_t function_proposed_to_kernel,
    struct acr_avaliable_functions *const functions) {

  size_t next_good = most_recent_function;
  bool good_function = false;
  while (!good_function) {
    next_good = (next_good+1) == functions->total_functions ? 0 : next_good+1;
    enum acr_avaliable_function_type funtype =
      atomic_load_explicit(
        &functions->function_priority[next_good]->type,
        memory_order_relaxed);
    switch (funtype) {
      case acr_function_empty:
      case acr_function_finished_cloog_gen:
#ifdef TCC_PRESENT
      case acr_function_tcc_and_shared:
#else
      case acr_function_shared_object_lib:
#endif
        good_function = next_good != function_used_by_kernel &&
          next_good != function_proposed_to_kernel;
        break;
      default:
        break;
    }
  }
  most_recent_function =
    (most_recent_function+1) == functions->total_functions ? 0 : most_recent_function+1;
  if (most_recent_function != next_good) {
    struct func_value *const temp = functions->function_priority[next_good];
    functions->function_priority[next_good] =
      functions->function_priority[most_recent_function];
    functions->function_priority[most_recent_function] = temp;
  }
  return most_recent_function;
}

static void acr_kernel_stencil(
    struct acr_runtime_data *const init_data,
    struct acr_avaliable_functions *const functions,
    struct acr_monitoring_computation *const monitor_data,
    struct acr_runtime_threads_compile_data *const compile_threads_data,
    struct acr_runtime_threads_cloog_gencode *const cloog_thread_data) {

  unsigned char *valid_monitor_result = NULL;
  unsigned char *invalid_monitor_result =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data->shared_buffer->scrap_values));
  unsigned char *maximized_version =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data->shared_buffer->scrap_values));
  size_t most_recent_function = 0;
  size_t function_proposed_to_kernel = 0;
  size_t function_used_by_kernel = functions->total_functions - 1;
  enum acr_kernel_function_type function_used_by_kernel_type =
    acr_kernel_function_initial;

  for (size_t i = 0; i < functions->total_functions; ++i) {
    functions->value[i].monitor_untouched =
      malloc(init_data->monitor_total_size *
          sizeof(*functions->value[i].monitor_untouched));
  }

  do {
    acr_get_most_recent_monitor_result(&valid_monitor_result,
                                      &invalid_monitor_result,
                                      monitor_data);
  } while (valid_monitor_result == NULL);

  pthread_mutex_lock(&cloog_thread_data->mutex);
  acr_cloog_compilation(&valid_monitor_result,
                        &invalid_monitor_result,
                        most_recent_function,
                        functions,
                        cloog_thread_data);
  memcpy(functions->function_priority[0]->monitor_untouched,
      functions->function_priority[0]->monitor_result,
      init_data->monitor_total_size);

  enum acr_avaliable_function_type most_recent_function_type;

  bool monitor_still_valid = false, validity;
  pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
  while (atomic_flag_test_and_set_explicit(
        &init_data->monitor_thread_continue, memory_order_relaxed)) {
    bool required_compilation;

    acr_get_most_recent_monitor_result(&valid_monitor_result,
                                       &invalid_monitor_result,
                                       monitor_data);

    if (valid_monitor_result == NULL) {
      valid_monitor_result = invalid_monitor_result;
      invalid_monitor_result = NULL;
      monitor_still_valid = true;
    } else {
      monitor_still_valid = false;
      acr_verify_2dstencil(
          (unsigned char) init_data->num_alternatives - 1,
          init_data->monitor_dim_max,
          valid_monitor_result,
          functions->function_priority[most_recent_function]->monitor_result,
          maximized_version, &required_compilation, &validity);
    }

    if (validity) {

      if (function_used_by_kernel != most_recent_function ||
          function_used_by_kernel_type != acr_kernel_function_using_cc) {

        most_recent_function_type =
          atomic_load_explicit(&functions->function_priority[most_recent_function]->type, memory_order_acquire);

        acr_valid_function_switch_to(most_recent_function_type,
            &function_used_by_kernel_type,
            &function_used_by_kernel,
            &function_proposed_to_kernel,
            most_recent_function,
            init_data,
            functions,
            compile_threads_data);
      } else {
        pthread_mutex_lock(&sleep_mutex);
        pthread_cond_wait(&init_data->coordinator_continue_cond, &sleep_mutex);
        pthread_mutex_unlock(&sleep_mutex);
      }

      if (!monitor_still_valid && required_compilation) {
        function_used_by_kernel_type = acr_kernel_function_initial;
        goto recompilation;
      }

      invalid_monitor_result = valid_monitor_result;
      valid_monitor_result = NULL;
    } else { // Version no more suitable
      if (monitor_still_valid) {
        invalid_monitor_result = valid_monitor_result;
        valid_monitor_result = NULL;
        continue;
      }

      discard_kernel_function(init_data, &function_used_by_kernel_type);

recompilation:
      most_recent_function_type = acr_function_empty;

      pthread_mutex_lock(&cloog_thread_data->mutex);
      bool cloog_has_to_generate = cloog_thread_data->generate_function;
      if (!(cloog_has_to_generate &&
          cloog_thread_data->where_to_add ==
          functions->function_priority[most_recent_function])) {
        if (cloog_has_to_generate)
          pthread_mutex_unlock(&cloog_thread_data->mutex);
        most_recent_function = acr_next_free_function_position(
            most_recent_function,
            function_used_by_kernel,
            function_proposed_to_kernel,
            functions);
        if (cloog_has_to_generate)
          pthread_mutex_lock(&cloog_thread_data->mutex);
      }

      invalid_monitor_result =
        functions->function_priority[most_recent_function]->monitor_untouched;
      functions->function_priority[most_recent_function]->monitor_untouched =
        valid_monitor_result;
      valid_monitor_result = maximized_version;
      maximized_version = invalid_monitor_result;
      acr_cloog_compilation(&valid_monitor_result,
          &invalid_monitor_result,
          most_recent_function,
          functions,
          cloog_thread_data);
    }
  }

  for (size_t i = 0; i < functions->total_functions; ++i) {
    free(functions->value[i].monitor_untouched);
  }
  free(valid_monitor_result);
  free(invalid_monitor_result);
  free(maximized_version);
}

static void acr_kernel_versioning(
    struct acr_runtime_data *const init_data,
    struct acr_avaliable_functions *const functions,
    struct acr_monitoring_computation *const monitor_data,
    struct acr_runtime_threads_compile_data *const compile_threads_data,
    struct acr_runtime_threads_cloog_gencode *const cloog_thread_data) {

  const double delta_treshold = 0.05;
  const size_t updated_version_treshold = 15;

  unsigned char *valid_monitor_result = NULL;
  unsigned char *invalid_monitor_result =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data->shared_buffer->scrap_values));
  unsigned char *maximized_version =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data->shared_buffer->scrap_values));
  size_t most_recent_function = 0;
  size_t function_proposed_to_kernel = 0;
  size_t function_used_by_kernel = functions->total_functions - 1;
  enum acr_kernel_function_type function_used_by_kernel_type =
    acr_kernel_function_initial;

  do {
    acr_get_most_recent_monitor_result(&valid_monitor_result,
                                      &invalid_monitor_result,
                                      monitor_data);
  } while (valid_monitor_result == NULL);

  pthread_mutex_lock(&cloog_thread_data->mutex);
  acr_cloog_compilation(&valid_monitor_result,
                        &invalid_monitor_result,
                        most_recent_function,
                        functions,
                        cloog_thread_data);

  size_t num_updated_version = 0;
  size_t total_version_update = 0;

  enum acr_avaliable_function_type most_recent_function_type;
  bool is_monitor_still_accurate, validity;
  pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
  while (atomic_flag_test_and_set_explicit(
        &init_data->monitor_thread_continue, memory_order_relaxed)) {

    acr_get_most_recent_monitor_result(&valid_monitor_result,
                                       &invalid_monitor_result,
                                       monitor_data);

    double delta;
    if (valid_monitor_result == NULL) {
      valid_monitor_result = invalid_monitor_result;
      invalid_monitor_result = NULL;
      is_monitor_still_accurate = true;
    } else {
      is_monitor_still_accurate = false;
      acr_verify_versioning(init_data->monitor_total_size,
          functions->function_priority[most_recent_function]->monitor_result,
          valid_monitor_result, maximized_version, init_data->num_alternatives,
          &delta, &validity);
    }

    if (validity) {

      if (function_used_by_kernel != most_recent_function ||
          function_used_by_kernel_type != acr_kernel_function_using_cc) {

        most_recent_function_type = atomic_load_explicit(&functions->function_priority[most_recent_function]->type, memory_order_acquire);

        acr_valid_function_switch_to(most_recent_function_type,
            &function_used_by_kernel_type,
            &function_used_by_kernel,
            &function_proposed_to_kernel,
            most_recent_function,
            init_data,
            functions,
            compile_threads_data);
      } else {
        pthread_mutex_lock(&sleep_mutex);
        pthread_cond_wait(&init_data->coordinator_continue_cond, &sleep_mutex);
        pthread_mutex_unlock(&sleep_mutex);
      }

      invalid_monitor_result = valid_monitor_result;
      valid_monitor_result = NULL;
    } else { // Version no more suitable
      if (is_monitor_still_accurate) {
        invalid_monitor_result = valid_monitor_result;
        valid_monitor_result = NULL;
        continue;
      }

      discard_kernel_function(init_data, &function_used_by_kernel_type);
      most_recent_function_type = acr_function_empty;

      if (num_updated_version < updated_version_treshold ||
          delta < delta_treshold) { // Not changing version
        num_updated_version += 1;
        total_version_update += 1;
        /*fprintf(stderr, "Update current version\n");*/
        invalid_monitor_result = valid_monitor_result;
        valid_monitor_result = maximized_version;
        maximized_version = invalid_monitor_result;
        invalid_monitor_result = NULL;
      } else {
        num_updated_version = 0;
        /*fprintf(stderr, "Changing version no more suitable %f\n", delta);*/
      }

      pthread_mutex_lock(&cloog_thread_data->mutex);
      bool cloog_has_to_generate = cloog_thread_data->generate_function;
      if (!(cloog_has_to_generate &&
          cloog_thread_data->where_to_add ==
          functions->function_priority[most_recent_function])) {
        if (cloog_has_to_generate)
          pthread_mutex_unlock(&cloog_thread_data->mutex);
        most_recent_function = acr_next_free_function_position(
            most_recent_function,
            function_used_by_kernel,
            function_proposed_to_kernel,
            functions);
        if (cloog_has_to_generate)
          pthread_mutex_lock(&cloog_thread_data->mutex);
      }

      acr_cloog_compilation(&valid_monitor_result,
          &invalid_monitor_result,
          most_recent_function,
          functions,
          cloog_thread_data);
    }
  }

  fprintf(stderr, "Total version update %zu\n", total_version_update);

  free(valid_monitor_result);
  free(invalid_monitor_result);
  free(maximized_version);
}

static void acr_kernel_simple(
    struct acr_runtime_data *const init_data,
    struct acr_avaliable_functions *const functions,
    struct acr_monitoring_computation *const monitor_data,
    struct acr_runtime_threads_compile_data *const compile_threads_data,
    struct acr_runtime_threads_cloog_gencode *const cloog_thread_data) {

  unsigned char *valid_monitor_result = NULL;
  unsigned char *invalid_monitor_result =
    malloc(init_data->monitor_total_size * sizeof(*monitor_data->shared_buffer->scrap_values));
  size_t most_recent_function = 0;
  size_t function_used_by_kernel = functions->total_functions - 1;
  enum acr_kernel_function_type function_used_by_kernel_type =
    acr_kernel_function_initial;

  do {
    acr_get_most_recent_monitor_result(&valid_monitor_result,
                                      &invalid_monitor_result,
                                      monitor_data);
  } while (valid_monitor_result == NULL);

  pthread_mutex_lock(&cloog_thread_data->mutex);
  acr_cloog_compilation(&valid_monitor_result,
                              &invalid_monitor_result,
                              most_recent_function,
                              functions,
                              cloog_thread_data);

  pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
  bool is_monitor_still_accurate, validity;
  while (atomic_flag_test_and_set_explicit(
        &init_data->monitor_thread_continue, memory_order_relaxed)) {

    acr_get_most_recent_monitor_result(&valid_monitor_result,
                                       &invalid_monitor_result,
                                       monitor_data);

    if (valid_monitor_result == NULL) {
      valid_monitor_result = invalid_monitor_result;
      invalid_monitor_result = NULL;
      is_monitor_still_accurate = true;
    } else {
      is_monitor_still_accurate = false;
      validity = acr_verify_me(init_data->monitor_total_size,
          functions->function_priority[most_recent_function]->monitor_result,
          valid_monitor_result);
    }

    // Test current function validity
    if (validity) { // Valid function

      if (function_used_by_kernel != most_recent_function ||
          function_used_by_kernel_type != acr_kernel_function_using_cc) {
      enum acr_avaliable_function_type type;
      type = atomic_load_explicit(&functions->function_priority[most_recent_function]->type, memory_order_acquire);

      size_t no_care;
      acr_valid_function_switch_to(type,
          &function_used_by_kernel_type,
          &function_used_by_kernel,
          &no_care,
          most_recent_function,
          init_data,
          functions,
          compile_threads_data);
      } else {
        pthread_mutex_lock(&sleep_mutex);
        pthread_cond_wait(&init_data->coordinator_continue_cond, &sleep_mutex);
        pthread_mutex_unlock(&sleep_mutex);
      }

      invalid_monitor_result = valid_monitor_result;
      valid_monitor_result = NULL;
    } else {
      if (is_monitor_still_accurate) {
        invalid_monitor_result = valid_monitor_result;
        valid_monitor_result = NULL;
        continue;
      }

      discard_kernel_function(init_data, &function_used_by_kernel_type);

      bool cloog_was_ready = true;
      pthread_mutex_lock(&cloog_thread_data->mutex);
      if(cloog_thread_data->num_threads ==
          cloog_thread_data->num_threads_compiling) { // All CLooGs are busy
        cloog_was_ready = false;
      }
      bool cloog_has_to_generate = cloog_thread_data->generate_function;
      if (!(cloog_has_to_generate &&
          cloog_thread_data->where_to_add ==
          functions->function_priority[most_recent_function])) {
        if (cloog_has_to_generate)
          pthread_mutex_unlock(&cloog_thread_data->mutex);
        most_recent_function = acr_next_free_function_position(
            most_recent_function,
            function_used_by_kernel,
            function_used_by_kernel,
            functions);
        if (cloog_has_to_generate)
          pthread_mutex_lock(&cloog_thread_data->mutex);
      }

      while(!cloog_was_ready && cloog_thread_data->num_threads ==
          cloog_thread_data->num_threads_compiling) { // All CLooGs are busy
        pthread_cond_wait(&cloog_thread_data->coordinator_sleep,
            &cloog_thread_data->mutex);
      }

      if (!cloog_was_ready) { // if cloog was not finished so get most recent again
        invalid_monitor_result = valid_monitor_result;
        valid_monitor_result = NULL;
        acr_get_most_recent_monitor_result(&valid_monitor_result,
            &invalid_monitor_result,
            monitor_data);
        if (valid_monitor_result == NULL) {
          valid_monitor_result = invalid_monitor_result;
          invalid_monitor_result = NULL;
        }
      }

      acr_cloog_compilation(&valid_monitor_result,
          &invalid_monitor_result,
          most_recent_function,
          functions,
          cloog_thread_data);
    }
  }
  free(valid_monitor_result);
  free(invalid_monitor_result);
}

void* acr_verification_and_coordinator_function(void *in_data) {
  struct acr_runtime_data *const init_data =
    (struct acr_runtime_data*) in_data;

  const size_t monitor_total_size = init_data->monitor_total_size;

  const size_t num_functions = 10;
  struct acr_avaliable_functions functions = {
    .total_functions = num_functions,
  };
  functions.value = malloc(functions.total_functions * sizeof(*functions.value));
  functions.function_priority = malloc(functions.total_functions *
      sizeof(*functions.function_priority));
  for (size_t i = 0; i < functions.total_functions; ++i) {
#ifdef TCC_PRESENT
    functions.value[i].compiler_specific.tcc.state = NULL;
#endif
    functions.value[i].compiler_specific.shared_obj_lib.dlhandle = NULL;
    atomic_store(&functions.value[i].type, acr_function_empty);
    functions.value[i].monitor_result =
      malloc(monitor_total_size *
          sizeof(*functions.value[i].monitor_result));
    functions.function_priority[i] = &functions.value[i];
    functions.value[i].memstream =
      open_memstream(&functions.value[i].generated_code,
          &functions.value[i].sizeof_string);
  }

  // Monitoring thread
  pthread_t monitoring_thread;
  struct acr_monitoring_shared shared_monitor_data = {
    .current_valid_computation = NULL,
    .scrap_values = NULL,
  };
  struct acr_monitoring_computation monitor_data = {
    .kernel_info = init_data->kernel_info,
    .monitoring_function = init_data->monitoring_function,
    .shared_buffer = &shared_monitor_data,
    .monitor_result_size = monitor_total_size,
    .end_yourself = ATOMIC_FLAG_INIT,
    .sleep_cond = &init_data->monitor_sleep_cond,
    .coordinator_continue_cond = &init_data->coordinator_continue_cond,
#ifdef ACR_STATS_ENABLED
    .num_mesurement = 0,
    .total_time = 0.,
#endif
  };
  atomic_flag_test_and_set(&monitor_data.end_yourself);
  monitor_data.shared_buffer->scrap_values =
    malloc(monitor_total_size * sizeof(*monitor_data.shared_buffer->scrap_values));
  pthread_create(&monitoring_thread, NULL, acr_runtime_monitoring_function,
      (void*) &monitor_data);

  // Compile threads
  const size_t num_compilation_threads = init_data->num_compile_threads;
  struct acr_runtime_threads_compile_data compile_threads_data = {
    .num_cflags = init_data->num_compiler_flags,
    .end_yourself = false,
    .compile_something = false,
    .num_threads = num_compilation_threads,
    .num_threads_compiling = num_compilation_threads,
    .coordinator_continue_cond = &init_data->coordinator_continue_cond,
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
    compile_threads_data.cflags = init_data->compiler_flags[i];
    pthread_create(&compile_threads[i], NULL,
        acr_runtime_compile_thread, (void*)&compile_threads_data);
  }

  // Cloog thread
  const size_t num_cloog_threads = init_data->num_codegen_threads; // We need to fix that
  pthread_t *cloog_threads =
    malloc(num_cloog_threads * sizeof(*compile_threads));
  struct acr_runtime_threads_cloog_gencode cloog_thread_data = {
    .thread_num = 0,
    .end_yourself = false,
    .generate_function = false,
    .num_threads = num_cloog_threads,
    .num_threads_compiling = num_cloog_threads,
    .coordinator_continue_cond = &init_data->coordinator_continue_cond,
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

  switch (init_data->kernel_strategy_type) {
    case acr_kernel_strategy_simple:
      acr_kernel_simple(init_data,
          &functions, &monitor_data, &compile_threads_data, &cloog_thread_data);
      break;
    case acr_kernel_strategy_versioning:
      acr_kernel_versioning(init_data,
          &functions, &monitor_data, &compile_threads_data, &cloog_thread_data);
      break;
    case acr_kernel_strategy_stencil:
      acr_kernel_stencil(init_data,
          &functions, &monitor_data, &compile_threads_data, &cloog_thread_data);
      break;
    case acr_kernel_strategy_unknown:
    default:
      fprintf(stderr, "Warning unknown strategy number %d\n",
          init_data->kernel_strategy_type);
      exit(1);
      break;
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
  atomic_flag_clear(&monitor_data.end_yourself);
  pthread_cond_signal(&init_data->monitor_sleep_cond);
  pthread_join(monitoring_thread, NULL);
  free(cloog_threads);

  // Clean all
  for (size_t i = 0; i < functions.total_functions; ++i) {
    fclose(functions.value[i].memstream);
    free(functions.value[i].generated_code);
    free(functions.value[i].monitor_result);
    if (functions.value[i].compiler_specific.shared_obj_lib.dlhandle)
        dlclose(functions.value[i].
            compiler_specific.shared_obj_lib.dlhandle);
#ifdef TCC_PRESENT
    if (functions.value[i].compiler_specific.tcc.state)
        tcc_delete(functions.value[i].compiler_specific.tcc.state);
#endif
  }

#ifdef ACR_STATS_ENABLED
    init_data->acr_stats->thread_stats.num_measurements[acr_thread_time_cloog] =
      cloog_thread_data.num_mesurement;
    init_data->acr_stats->thread_stats.total_time[acr_thread_time_cloog] =
      cloog_thread_data.total_time;
    init_data->acr_stats->thread_stats.num_measurements[acr_thread_time_monitor] =
      monitor_data.num_mesurement;
    init_data->acr_stats->thread_stats.total_time[acr_thread_time_monitor] =
      monitor_data.total_time;
    init_data->acr_stats->thread_stats.num_measurements[acr_thread_time_cc] =
      compile_threads_data.num_mesurement;
    init_data->acr_stats->thread_stats.total_time[acr_thread_time_cc] =
      compile_threads_data.total_time;
    init_data->acr_stats->thread_stats.num_measurements[acr_thread_time_tcc] =
      compile_threads_data.num_tcc_mesurement;
    init_data->acr_stats->thread_stats.total_time[acr_thread_time_tcc] =
      compile_threads_data.total_tcc_time;
#endif

  free(functions.value);
  free(functions.function_priority);
  pthread_exit(NULL);
}

static void* acr_cloog_generate_code_from_alt(void* in_data) {
  struct acr_runtime_threads_cloog_gencode *const input_data =
    (struct acr_runtime_threads_cloog_gencode *) in_data;

  pthread_mutex_lock(&input_data->mutex);
  const size_t thread_num = input_data->thread_num;
  input_data->thread_num += 1;
  pthread_mutex_unlock(&input_data->mutex);

#ifdef ACR_STATS_ENABLED
  double total_time = 0.;
  size_t num_mesurement = 0;
#endif

  FILE* stream;
  isl_set **generation_buffer =
    malloc(input_data->rdata->num_alternatives * sizeof(*generation_buffer));

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
    atomic_store(&where_to_add->type, acr_function_started_cloog_gen);

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
    fprintf(stream, "#include \"acr_required_definitions.h\"\n"
        "void acr_alternative_function%s {\n",
        input_data->rdata->function_prototype);
    acr_cloog_generate_alternative_code_from_input(stream, input_data->rdata,
        monitor_result, thread_num, generation_buffer);
    fprintf(stream, "}\n%c", '\0');

    // Now the pointers in function structure are up to date
    fflush(stream);

    atomic_store_explicit(&where_to_add->type, acr_function_finished_cloog_gen,
        memory_order_release);
    pthread_cond_signal(input_data->coordinator_continue_cond);

    /*fprintf(stderr, "%s\n", where_to_add->generated_code);*/

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
  free(generation_buffer);

  pthread_exit(NULL);
}

#ifdef TCC_PRESENT
static void* acr_runtime_compile_tcc(void* in_data) {
  struct acr_runtime_threads_compile_tcc *const input_data =
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

    TCCState *old_tccstate = where_to_add->compiler_specific.tcc.state;
    where_to_add->compiler_specific.tcc.state =
      tccstate;
    where_to_add->tcc_function = function;
      enum acr_avaliable_function_type t = acr_function_proposed_compilation;
      if (! atomic_compare_exchange_strong_explicit(
          &where_to_add->type,
          &t,
          acr_function_tcc_in_memory,
          memory_order_release,
          memory_order_relaxed)) {
        atomic_store_explicit(
            &where_to_add->type,
            acr_function_tcc_and_shared,
            memory_order_release);
      }
      pthread_cond_signal(input_data->coordinator_continue_cond);

#ifdef ACR_STATS_ENABLED
    acr_time tend;
    acr_get_current_time(&tend);
    total_time += acr_difftime(tstart, tend);
    num_mesurement += 1;
#endif

    if (old_tccstate != NULL) {
      tcc_delete(old_tccstate);
    }

  }

#ifdef ACR_STATS_ENABLED
  input_data->total_time = total_time;
  input_data->num_mesurement = num_mesurement;
#endif

  pthread_exit(NULL);
}
#endif

static void* acr_runtime_compile_thread(void* in_data) {
  struct acr_runtime_threads_compile_data *const input_data =
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
  tcc_data.coordinator_continue_cond = input_data->coordinator_continue_cond;
  pthread_mutex_init(&tcc_data.mutex, NULL);
  pthread_cond_init(&tcc_data.waking_up, NULL);
  pthread_create(&tcc_thread, NULL, acr_runtime_compile_tcc, (void*)&tcc_data);
#endif

  for (;;) {
    bool has_to_stop;

    pthread_mutex_lock(&input_data->mutex);

    if(input_data->num_threads_compiling == input_data->num_threads ||
        input_data->compile_something) {
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
      acr_compile_with_system_compiler(
          NULL,
          where_to_add->generated_code,
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

    void *old_dlhandle = where_to_add->compiler_specific.shared_obj_lib.dlhandle;
    where_to_add->
      compiler_specific.shared_obj_lib.dlhandle = dlhandle;
    where_to_add->cc_function = function;

    enum acr_avaliable_function_type t = acr_function_proposed_compilation;
    if (! atomic_compare_exchange_strong_explicit(
          &where_to_add->type,
          &t,
          acr_function_shared_object_lib,
          memory_order_release,
          memory_order_relaxed)) {
      atomic_store_explicit(
          &where_to_add->type,
          acr_function_tcc_and_shared,
          memory_order_release);
    }
    pthread_cond_signal(input_data->coordinator_continue_cond);
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

    if (old_dlhandle)
      dlclose(old_dlhandle);

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

void* acr_runtime_perf_compile_time_zero(void* in_data) {
  struct acr_runtime_perf *perf = (struct acr_runtime_perf*) in_data;

  struct acr_runtime_data * const rdata = perf->rdata;

  // Monitoring thread
  pthread_t monitoring_thread;
  struct acr_monitoring_shared shared_monitor_data = {
    .current_valid_computation = NULL,
    .scrap_values = NULL,
  };
  struct acr_monitoring_computation monitor_data = {
    .monitoring_function = rdata->monitoring_function,
    .shared_buffer = &shared_monitor_data,
    .monitor_result_size = rdata->monitor_total_size,
    .end_yourself = ATOMIC_FLAG_INIT,
#ifdef ACR_STATS_ENABLED
    .num_mesurement = 0,
    .total_time = 0.,
#endif
  };
  atomic_flag_test_and_set(&monitor_data.end_yourself);

  monitor_data.shared_buffer->scrap_values =
    malloc(rdata->monitor_total_size * sizeof(*monitor_data.shared_buffer->scrap_values));
  pthread_create(&monitoring_thread, NULL, acr_runtime_monitoring_function,
      (void*) &monitor_data);

  unsigned char *invalid_monitor_result =
    malloc(rdata->monitor_total_size * sizeof(*invalid_monitor_result));
  unsigned char *valid_monitor_result = NULL;

  const struct acr_performance_list *current_element = perf->compilation_list;

  while (atomic_flag_test_and_set_explicit(
        &rdata->monitor_thread_continue, memory_order_relaxed) &&
      current_element != NULL) {
    while (valid_monitor_result == NULL) {
      acr_get_most_recent_monitor_result(&valid_monitor_result,
                                        &invalid_monitor_result,
                                        &monitor_data);
    }
    if (acr_verify_me(rdata->monitor_total_size,
          current_element->element.monitor_result,
          valid_monitor_result)) { // Valid function

      atomic_store_explicit(
          &rdata->alternative_function,
          current_element->element.function,
          memory_order_relaxed);
      atomic_store_explicit(
          &rdata->current_monitoring_data,
          current_element->element.monitor_result,
          memory_order_relaxed);
    } else {
      atomic_store_explicit(
          &rdata->alternative_function,
          rdata->original_function,
          memory_order_relaxed);
      atomic_store_explicit(
          &rdata->current_monitoring_data,
          NULL,
          memory_order_relaxed);
      current_element = current_element->next;
    }
    invalid_monitor_result = valid_monitor_result;
    valid_monitor_result = NULL;
  }

  atomic_flag_clear(&monitor_data.end_yourself);


  pthread_join(monitoring_thread, NULL);
  free(invalid_monitor_result);

  pthread_exit(NULL);
}
