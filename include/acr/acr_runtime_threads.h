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

#ifndef __ACR_RUNTIME_THREADS_H
#define __ACR_RUNTIME_THREADS_H

#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#include "acr/acr_runtime_build.h"
#include "acr/acr_runtime_data.h"

enum acr_avaliable_function_type {
  acr_function_empty,
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
  pthread_spinlock_t spinlock;
  bool end_yourself;
};

struct acr_runtime_threads_cloog_gencode {
  size_t num_threads;
  size_t num_threads_compiling;
  struct func_value *where_to_add;
  struct acr_runtime_data *rdata;
  unsigned char* monitor_result;
  size_t monitor_total_size;
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
  pthread_mutex_t mutex;
  pthread_cond_t compiler_thread_sleep;
  pthread_cond_t coordinator_sleep;
  bool compile_something;
  bool end_yourself;
};

#ifdef TCC_PRESENT
struct acr_runtime_threads_compile_tcc {
  char *generated_code;
  struct func_value *where_to_add;
  pthread_mutex_t mutex;
  pthread_cond_t waking_up;
  bool compile_something;
  bool end_yourself;
};
#endif

void* acr_runtime_monitoring_function(void* in_data);

void* acr_runtime_compile_thread(void* in_data);

void* acr_cloog_generate_code_from_alt(void* in_data);

void* acr_verification_and_coordinator_function(void *in_data);

#endif // __ACR_RUNTIME_THREADS_H
