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

/**
 * \brief The type of performance test to do
 */
enum acr_runtime_perf_type {
  /** \brief Generate a C function doing all the kernel computation at once. */
  acr_runtime_perf_kernel_only,
  /** \brief Generate a shared object for each version used by the kernel */
  acr_runtime_perf_compilation_time_zero,
  /** \brief Load and run all the generated functions from
   * ::acr_runtime_perf_compilation_time_zero
   */
  acr_runtime_perf_compilation_time_zero_run,
};

/**
 * \brief Store data about currently used functions
 */
struct acr_perf_compilation {
  /** \brief The starting simulation step from this function */
  size_t starting_at;
  /** \brief The ending simulation step from this function */
  size_t ending_at;
  /** \brief The file name where the function has been generated */
  char *compilation_file_name;
  /** \brief The loop body of the function */
  char *loop_body;
  /** \brief The monitor data used to generate the code */
  unsigned char *monitor_result;
  /** \brief Function handle used with dlopen */
  void *handle;
  /** \brief The function pointer */
  void *function;
};

/**
 * \brief The list of all the functions called during the performance run
 */
struct acr_performance_list {
  /** The next element in the list */
  struct acr_performance_list *next;
  /** The current element */
  struct acr_perf_compilation element;
};

/**
 * \brief Structure used when doing a runtime perf
 */
struct acr_runtime_perf {
  /** \brief Current runtime data */
  struct acr_runtime_data *rdata;
  /** \brief The list head for fast access */
  struct acr_performance_list *list_head;
  /** \brief The entire list starting at the end */
  struct acr_performance_list *compilation_list;
  /** \brief The size of the list */
  size_t list_size;
  /** \brief The current performance test type */
  enum acr_runtime_perf_type type;
};

/**
 * \brief Call when the function in use is no more suitable
 * \param[in,out] perf The performance data structure
 * \return The pointer to the newly generated function
 *
 * Causes a recompilation of the function and must be called before running the
 * kernel again.
 */
void* acr_runntime_perf_end_step(struct acr_runtime_perf *perf);

/**
 * \brief Clean the performance data structure
 * \param[in,out] perf The performance data structure
 */
void acr_runtime_perf_clean(struct acr_runtime_perf *perf);

/**
 * \brief Print the kernel function call in case of
 * ::acr_runtime_perf_kernel_only type test
 * \param[in,out] output The output stream
 * \param[in] perf The performance structure after the run
 */
void acr_runtime_print_perf_kernel_function_call(
    FILE* output,
    struct acr_runtime_perf *perf);

/**
 * \brief Print the initialization function used by a successive test to
 * ::acr_runtime_perf_compilation_time_zero_run
 * \param[in,out] output The output stream
 * \param[in] perf The performance structure after the run
 */
void acr_runtime_print_perf_compile_time_zero_function_list_init(
    FILE* output,
    struct acr_runtime_perf *perf);

/**
 * \brief Add an element to the runtime performance list
 * \param[in,out] perf The performance structure where to append the element
 * \param[in] compilation The element to add to the list
 */
void acr_perf_add_compilation_to_list(struct acr_runtime_perf *perf,
    struct acr_perf_compilation compilation);

/**
 * \brief Clean the performance structure after an
 * ::acr_runtime_perf_compilation_time_zero_run test
 * \param[in,out] perf The performance structure to free
 */
void acr_runtime_clean_time_zero_run(struct acr_runtime_perf *perf);

#endif // __ACR_RUNTIME_PERF_H

/**
 *
 * @}
 *
 */
