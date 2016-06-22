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
  /** The number of compiler flags */
  size_t num_compiler_flags;
  /** The compiler flags */
  char ***compiler_flags;

  /** The monitoring data used by the current kernel */
  unsigned char *current_monitoring_data;
  /** The alternative function the kernel should be using. NULL for default
   * function */
  void *alternative_function;
  /** Lock for thread mutual exclusion */
  pthread_spinlock_t alternative_lock;
  /** Number of time the alternative will still be viable (in function call
   * number */
  uint_fast8_t alternative_still_usable;
  /** The function time validity default value */
  uint_fast8_t usability_inital_value;
  /** The coordinator thread */
  pthread_t monitor_thread;
  /** The stop condition of the coordinator */
  bool monitor_thread_continue;
#ifdef ACR_STATS_ENABLED
  /** Statistics about running threads */
  struct acr_threads_time_stats thread_stats;
  /** Statistics about the computation kernel */
  struct acr_simulation_time_stats sim_stats;
#endif
  /** The kernel strategy type */
  enum acr_kernel_strategy_type kernel_strategy_type;
};

/**
 * \brief Data structure used in static kernel during runtime
 */
struct acr_runtime_data_static {
  /** \brief The number of functions */
  size_t total_functions;
  /** \brief The functions */
  void **functions;
  /** \brief The number of alternatives */
  size_t num_alternatives;
  /** \brief The alternatives array */
  struct runtime_alternative *alternatives;
  /** \brief The OpenScop format of the kernel */
  struct osl_scop *scop;
  /** \brief The first monitor dimension */
  size_t first_monitor_dimension;
  /** \brief The number of monitoring dimensions */
  size_t num_monitor_dimensions;
  /** \brief The tiling size */
  size_t grid_size;
  /** \brief The statements in CLooG representation */
  CloogUnionDomain *union_domain;
  /** \brief The context */
  CloogDomain *context;
  /** \brief The CLooG state */
  CloogState *state;
};

/**
 * \brief Initialize static runtime data structure
 * \param[in,out] static_data The targeted structure
 * \param[in] scop The OpenScop format from the kernel
 * \param[in] scop_size The number of characters of the scop string
 */
void init_acr_static_data(
    struct acr_runtime_data_static *static_data,
    char *scop,
    size_t scop_size);

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
 * \brief Return the grid size
 * \param[in] data The acr runtime data structure
 * \return The tiling/grid size
 */
size_t acr_runtime_get_grid_size(struct acr_runtime_data* data);

#endif // __ACR_RUNTIME_DATA_H

/**
 *
 * @}
 *
 */
