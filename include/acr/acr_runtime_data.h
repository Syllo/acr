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
 * \file acr_runtime_data.h
 *
 * \brief ACR runtime data structure
 *
 * \defgroup runtime_data
 *
 * @{
 *
 * \brief The ACR runtime data structure with initialization and cleaning functions.
 *
 */

#ifndef __ACR_RUNTIME_DATA_H
#define __ACR_RUNTIME_DATA_H

#include <acr/acr_openscop.h>
#include <acr/runtime_alternatives.h>
#include <cloog/cloog.h>
#include <isl/map.h>
#include <pthread.h>
#include "acr/acr_stats.h"
#include <stdatomic.h>

/**
 * \brief The strategy acr uses at runtime
 * \sa ::acr_runtime_kernel_version
 */
enum acr_kernel_strategy_type {
  /** \brief ACR simple strategy */
  acr_kernel_strategy_simple = 0,
  /** \brief ACR versioning strategy */
  acr_kernel_strategy_versioning,
  /** \brief ACR stencil strategy */
  acr_kernel_strategy_stencil,
  /** \brief Error value */
  acr_kernel_strategy_unknown,
};

/**
 * Runtime main thread timing infos
 */
struct acr_runtime_kernel_info {
  /** The number of kernel calls */
  size_t num_calls;
  /** The mean time between two kernel call */
  double sim_step_time;
  /** The temporaty time for a simulation step */
  acr_time step_temp_time;
};


/**
 * Statistics for acr
 */
struct acr_runtime_stats {
  /** Statistics about running threads */
  struct acr_threads_time_stats thread_stats;
  /** Statistics about the computation kernel */
  struct acr_simulation_time_stats sim_stats;
};

/**
 * \brief The main data structure having all the data needed at runtime
 */
struct acr_runtime_data {
  /** \brief Number of code generation threads */
  size_t num_codegen_threads;
  /** \brief Number of compilation threads */
  size_t num_compile_threads;
  /** The prefix used during compilation */
  char *kernel_prefix;
  /** The CLooG state for each threads */
  CloogState **state;
  /** The OpenScop relation used by code generation functions */
  struct osl_scop* osl_relation;
  /** The number of alternatives */
  size_t num_alternatives;
  /** The alternatives data */
  struct runtime_alternative *alternatives;
  /** The number of monitor dimensions */
  unsigned int num_monitor_dims;
  /** The upper bound of the dimension */
  unsigned long *monitor_dim_max;
  /** The pre-computed tiles used by CLooG to generate code */
  isl_set ***tiles_domains;
  /** An empty set used for performance */
  isl_set **empty_monitor_set;
  /** The size of the monitor data */
  size_t monitor_total_size;
  /** The tiling size */
  size_t grid_size;
  /** Number of statement of the kernel */
  size_t num_statements;
  /** The loop context */
  isl_set **context;
  /** The scattering function for each statements for each threads */
  isl_map ***statement_maps;
  /** An array storing the number of dimensions per statements */
  unsigned int *dimensions_per_statements;
  /** For each dimensions in each statements, it's type */
  enum acr_dimension_type **statement_dimension_types;
  /** The function prototype used for code generation */
  char* function_prototype;
  /** A pointer to a function giving the alternative structure for each cell */
  struct runtime_alternative* (*alternative_from_val)(unsigned char);
  /** The monitoring function pointer */
  void (*monitoring_function)(unsigned char*);
  /** The initial function pointer */
  void *original_function;
  /** The number of compiler flags */
  size_t num_compiler_flags;
  /** The compiler flags */
  char ***compiler_flags;
  pthread_cond_t monitor_sleep_cond;
  pthread_cond_t coordinator_continue_cond;

  /** The monitoring data used by the current kernel */
  _Atomic (unsigned char*) current_monitoring_data;
  /** The alternative function the kernel should be using. NULL for default
   * function */
  _Atomic (void *) alternative_function;
  /** The coordinator thread */
  pthread_t monitor_thread;
  /** The stop condition of the coordinator */
  atomic_flag monitor_thread_continue;
  /** Kernel informations */
  struct acr_runtime_kernel_info *kernel_info;
#ifdef ACR_STATS_ENABLED
  struct acr_runtime_stats *acr_stats;
#endif
  /** The kernel strategy type */
  enum acr_kernel_strategy_type kernel_strategy_type;
  bool generate_optimum_function;
};

/**
 * \brief The reduction function used for each tile
 */
enum acr_static_data_reduction_function {
  /** \brief Use minumum */
  acr_reduction_min,
  /** \brief Use maximum */
  acr_reduction_max,
  /** \brief Use average */
  acr_reduction_avg,
};

/**
 * \brief Data structure used in static kernel during runtime
 */
struct acr_runtime_data_static {
  /** \brief set as true if the not yet initialized */
  bool is_uninitialized;
  /** \brief The array where all the generated functions pointer are stored */
  void **all_functions;
  /** \brief The number of alternatives */
  size_t num_alternatives;
  /** \brief The alternatives */
  void **alternative_functions;
  /** \brief The number of monitoring dimensions */
  const size_t num_monitor_dimensions;
  /** \brief The tiling size */
  const size_t grid_size;
  /** \beirf Runtime lexicographic minimum and maximum */
  intmax_t (*min_max)[2];
  /** \brief The total size of the function array */
  size_t total_functions;
};

/**
 * \brief Free the data structure used by a static kernel
 * \param[in] static_data The targeted structure
 */
void free_acr_static_data(struct acr_runtime_data_static *static_data);

/**
 * \brief Initialize threads specific fields
 * \param[in,out] data The structure where the fields needs to be initialized
 */
void init_acr_runtime_data_thread_specific(struct acr_runtime_data *data);

/**
 * \brief Initialize all the structure for runtime use
 * \param[in,out] data The structure where the fields needs to be initialized
 * \param[in] scop The OpenScop that has beed used to generate the data
 * \param[in] scop_size The number of characters of the scop string
 */
void init_acr_runtime_data(
    struct acr_runtime_data* data,
    char *scop,
    size_t scop_size);

/**
 * \brief Clean the structure threads specific data
 * \param[in,out] data The structure where the fields needs to be cleaned
 */
void free_acr_runtime_data_thread_specific(struct acr_runtime_data* data);

/**
 * \brief Clean the structure after use
 * \param[in,out] data The structure where the fields needs to be cleaned
 */
void free_acr_runtime_data(struct acr_runtime_data* data);

/**
 * \brief Wrapper to get the runtime data
 * \param[in] data The acr runtime data structure
 * \retval NULL When the default function is used
 * \return A pointer to the currently used alternative grid.
 */
unsigned char* acr_runtime_get_runtime_data(struct acr_runtime_data* data);

/**
 * \brief Return the number of monitor dimensions
 * \param[in] data The acr runtime data structure
 * \return The number of monitoring dimensions
 */
size_t acr_runtime_get_num_monitor_dims(struct acr_runtime_data* data);

/**
 * \brief Get the monitoring dimensions upper bounds
 * \param[in] data The acr runtime data structure
 * \return The monitoring dimensions upper bounds pointer
 */
unsigned long* acr_runtime_get_monitor_dims_upper_bounds(
    struct acr_runtime_data* data);

/**
 * \brief Return the number of alternatives
 * \param[in] data The acr runtime data structure
 * \return The number of alternatives
 */
size_t acr_runtime_get_num_alternatives(struct acr_runtime_data*data);

/**
 * \brief Initialize the grid with static functions
 * \param[in,out] static_data The static data structure.
 */
void acr_static_data_init_grid(struct acr_runtime_data_static *static_data);

/**
 * \brief Initialize the compiler flags
 * \param[out] opt The compiler options to initialize
 * \param[out] num_opt The number of compiler options inside of opt
 */
void acr_compile_flags(char ***opt, size_t *num_opt);

/**
 * \brief Free compile flags
 * \param[in] flags The compile flags
 */
void acr_free_compile_flags(char **flags);

void acr_runtime_data_specialize_alternative_domain(
    struct acr_runtime_data *data, struct runtime_alternative *alt,
    size_t statement_id, isl_set **restricted_domains);

#endif // __ACR_RUNTIME_DATA_H

/**
 *
 * @}
 *
 */
