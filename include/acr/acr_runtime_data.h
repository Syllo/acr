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

struct acr_runtime_data {
  CloogState* state;
  struct osl_scop* osl_relation;
  size_t num_alternatives;
  struct runtime_alternative *alternatives;
  unsigned int num_monitor_dims;
  unsigned long *monitor_dim_max;
  isl_set **tiles_domains;
  isl_set *empty_monitor_set;
  size_t monitor_total_size;
  size_t grid_size;
  size_t num_statements;
  isl_set *context;
  isl_map **statement_maps;
  unsigned int *dimensions_per_statements;
  enum acr_dimension_type **statement_dimension_types;
  char* function_prototype;
  struct runtime_alternative* (*alternative_from_val)(unsigned char);
  void (*monitoring_function)(unsigned char*);
  size_t num_compiler_flags;
  char **compiler_flags;

  unsigned char *current_monitoring_data;
  pthread_spinlock_t alternative_lock;
  void *alternative_function;
  uint_fast8_t alternative_still_usable;
  uint_fast8_t usability_inital_value;
  pthread_t monitor_thread;
  bool monitor_thread_continue;
  char *kernel_prefix;
#ifdef ACR_STATS_ENABLED
  struct acr_threads_time_stats thread_stats;
  struct acr_simulation_time_stats sim_stats;
#endif
};

void init_acr_runtime_data_thread_specific(struct acr_runtime_data *data);

void init_acr_runtime_data(
    struct acr_runtime_data* data,
    char *scop,
    size_t scop_size);

void free_acr_runtime_data_thread_specific(struct acr_runtime_data* data);

void free_acr_runtime_data(struct acr_runtime_data* data);

unsigned char* acr_runtime_get_runtime_data(struct acr_runtime_data* data);

#endif // __ACR_RUNTIME_DATA_H

/**
 *
 * @}
 *
 */
