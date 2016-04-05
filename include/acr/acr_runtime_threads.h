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
};

struct acr_avaliable_functions {
  enum acr_avaliable_function_type type;
  pthread_mutex_t lock;
  size_t total_functions;
  size_t num_functions;
  union {
    struct {
      void *dlhandle;
      void *function;
    } shared_obj_lib;
    struct {
      TCCState *state;
      void *function;
    } tcc;
  } *value;
};

struct acr_runtime_threads_data {
  pthread_t monitoring_thread;
  pthread_cond_t monitoring_cond;
  pthread_mutex_t monitoring_has_finished;
  bool monitor_waiting;
};

struct acr_runtime_threads_monitor_init {
  struct acr_runtime_data *rdata;
  void (*monitor_function)(unsigned char* monitor_result);
  pthread_cond_t *waiting_cond;
  pthread_mutex_t *waiting_when_finished;
  bool *waiting;
  struct acr_avaliable_functions *functions;
};

struct acr_runtime_threads_compile_data {
  unsigned char* monitor_result;
  struct acr_runtime_data *rdata;
};

void* acr_runtime_monitoring_function(void* monitoring_function);

void* acr_runtime_compile_thread(void* in_data);

#endif // __ACR_RUNTIME_THREADS_H
