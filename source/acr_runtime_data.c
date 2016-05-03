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

#include "acr/acr_runtime_data.h"
#include "acr/cloog_runtime.h"
#include "acr/osl_runtime.h"
#include "acr/acr_runtime_build.h"

#include <cloog/isl/domain.h>
#include <isl/constraint.h>
#include <isl/set.h>
#include <isl/map.h>
#include <pthread.h>
#include <string.h>

void free_acr_runtime_data(struct acr_runtime_data* data) {
  data->monitor_thread_continue = false;
  pthread_join(data->monitor_thread, NULL);
  for (size_t i = 0; i < data->num_alternatives; ++i) {
    struct runtime_alternative *alt = &data->alternatives[i];
    for (size_t j = 0; j < data->num_statements; ++j) {
      isl_set_free(alt->restricted_domains[j]);
    }
    free(alt->restricted_domains);
  }
  for (size_t i = 0; i < data->num_statements; ++i) {
    isl_map_free(data->statement_maps[i]);
  }
  for (unsigned long i = 0; i < data->monitor_total_size; ++i) {
    isl_set_free(data->tiles_domains[i]);
  }
  free(data->tiles_domains);
  isl_set_free(data->empty_monitor_set);
  free(data->statement_maps);
  isl_set_free(data->context);
  cloog_state_free(data->state);
  data->state = NULL;
  osl_scop_free(data->osl_relation);
  data->osl_relation = NULL;
  pthread_spin_destroy(&data->alternative_lock);
  free(data->monitor_dim_max);
  free(data->compiler_flags[1]);
  free(data->compiler_flags);
}

isl_map* isl_map_from_cloog_scattering(CloogScattering *scat);

static void init_isl_tiling_domain(struct acr_runtime_data *data) {
  data->tiles_domains =
    malloc(data->monitor_total_size * sizeof(*data->tiles_domains));

  isl_ctx *ctx = isl_set_get_ctx(data->context);
  isl_space *space = isl_space_set_alloc(ctx, 0, data->num_monitor_dims);
  isl_val *tiling_size_val = isl_val_int_from_ui(ctx, data->grid_size);

  isl_set *empty_domain = isl_set_empty(isl_space_copy(space));
  data->empty_monitor_set = empty_domain;

  isl_set *universe_domain = isl_set_universe(space);

  unsigned long *current_dimension =
    calloc(data->num_monitor_dims, sizeof(*current_dimension));
  for(size_t i = 0; i < data->monitor_total_size; ++i) {
    data->tiles_domains[i] = isl_set_copy(universe_domain);
    for (unsigned long j = 0; j < data->num_monitor_dims; ++j) {
      unsigned long dimensions_pos = current_dimension[data->num_monitor_dims- 1 - j];
      isl_local_space *local_space =
        isl_local_space_from_space(isl_set_get_space(data->tiles_domains[i]));
      isl_constraint *c_lower = isl_constraint_alloc_inequality(
          local_space);
      isl_constraint *c_upper = isl_constraint_copy(c_lower);

      isl_val *lower_bound =
        isl_val_mul_ui(isl_val_copy(tiling_size_val), dimensions_pos);
      lower_bound = isl_val_neg(lower_bound);
      c_lower =
        isl_constraint_set_constant_val(c_lower, lower_bound);
      c_lower =
        isl_constraint_set_coefficient_si(c_lower, isl_dim_set, (int)j, 1);
      data->tiles_domains[i] =
        isl_set_add_constraint(data->tiles_domains[i], c_lower);

      isl_val *upper_bound =
        isl_val_mul_ui(isl_val_copy(tiling_size_val), dimensions_pos);
      upper_bound = isl_val_add(upper_bound, isl_val_copy(tiling_size_val));
      upper_bound = isl_val_sub_ui(upper_bound, 1);
      c_upper =
        isl_constraint_set_constant_val(c_upper, upper_bound);
      c_upper =
        isl_constraint_set_coefficient_si(c_upper, isl_dim_set, (int)j, -1);
      data->tiles_domains[i] =
        isl_set_add_constraint(data->tiles_domains[i], c_upper);
    }

    for (unsigned long j = 0; j < data->num_monitor_dims; ++j) {
      current_dimension[j] += 1;
      if(current_dimension[j] == data->monitor_dim_max[j]) {
        current_dimension[j] = 0;
      } else {
        break;
      }
    }
  }
  isl_set_free(universe_domain);
  isl_val_free(tiling_size_val);
  free(current_dimension);
}

static void init_compile_flags(struct acr_runtime_data *data) {

  char *env_val = getenv("ACR_EXTRA_CFLAGS");
  char **options = NULL;
  size_t num_options = 0;
  if (env_val != NULL) {
    size_t env_lenght = strlen(env_val);
    char* env_val_copy = malloc((env_lenght+1) * sizeof(*env_val_copy));
    memcpy(env_val_copy, env_val, (env_lenght+1) * sizeof(char));

    char *current_option_start_pos = NULL;

    if (env_lenght > 0) {
      for (size_t i = 0; i < env_lenght; ++i) {
        if (current_option_start_pos == NULL) {
          if (env_val_copy[i] == ':') {
            fprintf(stderr, "Malformed ACR_EXTRA_CFLAGS, usign default one\n");
            free(options);
            free(env_val_copy);
            options = NULL;
            num_options = 0;
            goto default_flags;
          }
          current_option_start_pos = &env_val_copy[i];
        }
        if(env_val_copy[i] == ':') {
          num_options += 1;
          options = realloc(options, num_options * sizeof(*options));
          options[num_options-1] = current_option_start_pos;
          current_option_start_pos = NULL;
          env_val_copy[i] = '\0';
        }
      }
      if (current_option_start_pos) {
        num_options += 1;
        options = realloc(options, num_options * sizeof(*options));
        options[num_options-1] = current_option_start_pos;
      }
    } else {
      free(env_val_copy);
      goto default_flags;
    }
  } else {
default_flags:
    num_options = 1;
    options = malloc(sizeof(*options));
    options[0] = malloc(4*sizeof(char));
    memcpy(options[0], "-O2", 4*sizeof(char));
  }
  acr_append_necessary_compile_flags(&num_options, &options);
  data->compiler_flags = options;
  data->num_compiler_flags = num_options;
}

void init_acr_runtime_data(
    struct acr_runtime_data* data,
    char *scop,
    size_t scop_size) {
  data->osl_relation = acr_read_scop_from_buffer(scop, scop_size);
  data->state = cloog_state_malloc();
  data->monitor_total_size = 1;
  for (size_t i = 0; i < data->num_monitor_dims; ++i) {
    data->monitor_total_size *= data->monitor_dim_max[i];
  }
  pthread_spin_init(&data->alternative_lock, PTHREAD_PROCESS_PRIVATE);
  data->usability_inital_value = 3;

  CloogInput *cloog_input = cloog_input_from_osl_scop(data->state,
      data->osl_relation);
  acr_cloog_init_scop_to_match_alternatives(data);
  data->statement_maps =
    malloc(data->num_statements * sizeof(*data->statement_maps));
  CloogNamedDomainList *domain_list = cloog_input->ud->domain;
  for (size_t i = 0; i < data->num_statements; ++i, domain_list = domain_list->next) {
    data->statement_maps[i] = isl_map_copy(
        isl_map_from_cloog_scattering(domain_list->scattering));
  }
  for (size_t i = 0; i < data->num_alternatives; ++i) {
    struct runtime_alternative *alt = &data->alternatives[i];
    alt->restricted_domains =
      malloc(data->num_statements * sizeof(*alt->restricted_domains));
    domain_list = cloog_input->ud->domain;
    for(size_t j = 0; j < data->num_statements; ++j, domain_list = domain_list->next) {
      alt->restricted_domains[j] =
        isl_set_copy(isl_set_from_cloog_domain(domain_list->domain));
    }
  }
  data->context = isl_set_from_cloog_domain(cloog_input->context);
  cloog_union_domain_free(cloog_input->ud);
  free(cloog_input);
  init_isl_tiling_domain(data);
  init_compile_flags(data);
}

unsigned char* acr_runtime_get_runtime_data(struct acr_runtime_data* data) {
  return data->current_monitoring_data;
}

