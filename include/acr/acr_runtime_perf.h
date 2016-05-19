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

/**
 *
 * \file acr_runtime_perf.h
 * \brief ACR self performance analysis functions
 *
 * \defgroup runtime_perf
 *
 * @{
 * \brief ACR self performance analysis tools
 *
 */

#ifndef __ACR_RUNTIME_PERF_H
#define __ACR_RUNTIME_PERF_H

#include <stdint.h>

#include "acr/acr_runtime_data.h"

enum acr_runtime_perf_type {
  acr_runtime_perf_kernel_only,
  acr_runtime_perf_compilation_time_zero,
  acr_runtime_perf_compilation_time_zero_run,
};

struct acr_perf_compilation {
  size_t starting_at, ending_at;
  char *compilation_file_name;
  char *loop_body;
  unsigned char *monitor_result;
  void *handle;
  void *function;
};

struct acr_performance_list {
  struct acr_performance_list *next;
  struct acr_perf_compilation element;
};


struct acr_runtime_perf {
  struct acr_runtime_data *rdata;
  struct acr_performance_list *list_head;
  struct acr_performance_list *compilation_list;
  size_t list_size;
  enum acr_runtime_perf_type type;
};

void* acr_runntime_perf_end_step(struct acr_runtime_perf *perf);

void acr_runtime_perf_clean(struct acr_runtime_perf *perf);

void acr_runtime_print_perf_kernel_function_call(
    FILE* output,
    struct acr_runtime_perf *perf);

void acr_runtime_print_perf_compile_time_zero_function_list_init(
    FILE* output,
    struct acr_runtime_perf *perf);

void acr_perf_add_compilation_to_list(struct acr_runtime_perf *perf,
    struct acr_perf_compilation compilation);

void acr_runtime_clean_time_zero_run(struct acr_runtime_perf *perf);

#endif // __ACR_RUNTIME_PERF_H

/**
 *
 * @}
 *
 */
