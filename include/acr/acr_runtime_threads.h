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
  acr_function_shared_object_lib,
  acr_tcc_in_memory,
  acr_function_empty,
};

struct acr_avaliable_functions {
  size_t total_functions;
  size_t function_in_use;
  struct {
    pthread_spinlock_t lock;
    bool is_ready;
    enum acr_avaliable_function_type type;
    unsigned char *monitor_result;
    union {
      struct {
        void *dlhandle;
      } shared_obj_lib;
      struct {
        TCCState *state;
      } tcc;
    } compiler_specific;
    void *function;
  } *value;
};

struct acr_runtime_threads_compile_data {
  unsigned char* monitor_result;
  struct acr_runtime_data *rdata;
  struct acr_avaliable_functions *functions;
  size_t where_to_add;
};

struct acr_runtime_threads_data {
  pthread_t monitoring_thread;
  pthread_cond_t monitoring_cond;
  pthread_mutex_t monitoring_has_finished;
  bool monitor_waiting;
};


void* acr_runtime_monitoring_function(void* monitoring_function);

void* acr_runtime_compile_thread(void* in_data);

#endif // __ACR_RUNTIME_THREADS_H
