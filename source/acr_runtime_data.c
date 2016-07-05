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

#include "acr/acr_runtime_build.h"
#include "acr/acr_runtime_code_generation.h"
#include "acr/acr_runtime_data.h"
#include "acr/acr_runtime_osl.h"

#include <cloog/isl/domain.h>
#include <isl/constraint.h>
#include <isl/set.h>
#include <isl/map.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

void free_acr_runtime_data_thread_specific(struct acr_runtime_data* data) {
  pthread_spin_lock(&data->alternative_lock);
  data->monitor_thread_continue = false;
  pthread_spin_unlock(&data->alternative_lock);
  pthread_join(data->monitor_thread, NULL);
  pthread_spin_destroy(&data->alternative_lock);
}

void free_acr_runtime_data(struct acr_runtime_data* data) {
  for (size_t k = 0; k < data->num_codegen_threads; ++k) {
    for (size_t i = 0; i < data->num_alternatives; ++i) {
      struct runtime_alternative *alt = &data->alternatives[i];
      for (size_t j = 0; j < data->num_statements; ++j) {
        isl_set_free(alt->restricted_domains[k][j]);
      }
      free(alt->restricted_domains[k]);
    }
    for (size_t i = 0; i < data->num_statements; ++i) {
      isl_map_free(data->statement_maps[k][i]);
    }
    free(data->statement_maps[k]);
    for (unsigned long i = 0; i < data->monitor_total_size; ++i) {
      isl_set_free(data->tiles_domains[k][i]);
    }
    free(data->tiles_domains[k]);
    isl_set_free(data->context[k]);
    isl_set_free(data->empty_monitor_set[k]);
    cloog_state_free(data->state[k]);
  }
  for (size_t i = 0; i < data->num_alternatives; ++i) {
    struct runtime_alternative *alt = &data->alternatives[i];
    free(alt->restricted_domains);
  }
  free(data->context);
  free(data->state);
  free(data->statement_maps);
  free(data->tiles_domains);
  free(data->empty_monitor_set);
  data->state = NULL;
  osl_scop_free(data->osl_relation);
  data->osl_relation = NULL;
  free(data->monitor_dim_max);
  free(data->compiler_flags[0][1]);
  for (size_t i = 0; i < data->num_compile_threads; ++i)
    free(data->compiler_flags[i]);
  free(data->compiler_flags);
}

isl_map* isl_map_from_cloog_scattering(CloogScattering *scat);

static void init_isl_tiling_domain(struct acr_runtime_data *data) {

  data->tiles_domains =
    malloc(data->num_codegen_threads * sizeof(*data->tiles_domains));
  data->empty_monitor_set =
    malloc(data->num_codegen_threads * sizeof(*data->empty_monitor_set));
  for (size_t k = 0; k < data->num_codegen_threads; ++k) {
    data->tiles_domains[k] =
      malloc(data->monitor_total_size * sizeof(*data->tiles_domains[k]));

    isl_ctx *ctx = isl_set_get_ctx(data->context[k]);
    isl_space *space = isl_space_set_alloc(ctx, 0, data->num_monitor_dims);
    isl_val *tiling_size_val = isl_val_int_from_ui(ctx, data->grid_size);

    isl_set *empty_domain = isl_set_empty(isl_space_copy(space));
    data->empty_monitor_set[k] = empty_domain;

    isl_set *universe_domain = isl_set_universe(space);

    unsigned long *current_dimension =
      calloc(data->num_monitor_dims, sizeof(*current_dimension));
    for(size_t i = 0; i < data->monitor_total_size; ++i) {
      data->tiles_domains[k][i] = isl_set_copy(universe_domain);
      for (unsigned long j = 0; j < data->num_monitor_dims; ++j) {
        unsigned long dimensions_pos = current_dimension[data->num_monitor_dims- 1 - j];
        isl_local_space *local_space =
          isl_local_space_from_space(isl_set_get_space(data->tiles_domains[k][i]));
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
        data->tiles_domains[k][i] =
          isl_set_add_constraint(data->tiles_domains[k][i], c_lower);

        isl_val *upper_bound =
          isl_val_mul_ui(isl_val_copy(tiling_size_val), dimensions_pos);
        upper_bound = isl_val_add(upper_bound, isl_val_copy(tiling_size_val));
        upper_bound = isl_val_sub_ui(upper_bound, 1);
        c_upper =
          isl_constraint_set_constant_val(c_upper, upper_bound);
        c_upper =
          isl_constraint_set_coefficient_si(c_upper, isl_dim_set, (int)j, -1);
        data->tiles_domains[k][i] =
          isl_set_add_constraint(data->tiles_domains[k][i], c_upper);
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
  data->compiler_flags =
    malloc(data->num_compile_threads * sizeof(*data->compiler_flags));
  data->compiler_flags[0] = options;
  data->num_compiler_flags = num_options;
  for (size_t i = 1; i < data->num_compile_threads; ++i) {
    data->compiler_flags[i] =
      malloc(num_options * sizeof(*data->compiler_flags[i]));
    for (size_t j = 0; j < num_options; ++j) {
      data->compiler_flags[i][j] = data->compiler_flags[0][j];
    }
  }
}

void init_acr_runtime_data_thread_specific(struct acr_runtime_data *data) {
  pthread_spin_init(&data->alternative_lock, PTHREAD_PROCESS_PRIVATE);
}

/**
 * \brief Initialize the number of threads used during the simulation
 * \param[out] codegen The number of code generation threads
 * \param[out] compile The number of compilation threads
 *
 * \remark You can use the *ACR_GEN_THREADS* environment variable to set the
 * number of code generation threads.
 * \remark You can use the *ACR_COMPILE_THREADS* environment variable to set
 * the number of compilation threads.
 *
 */
static void init_num_threads(size_t *restrict codegen, size_t *restrict compile) {
  char *codegen_env = getenv("ACR_GEN_THREADS");
  long num_threads = sysconf(_SC_NPROCESSORS_ONLN);
  num_threads /= 2;
  num_threads = num_threads == 0 ? 1 : num_threads;
  if (codegen_env == NULL) {
    *codegen = (size_t) num_threads;
  } else {
    long env_threads;
    int num_matched = sscanf(codegen_env, "%ld", &env_threads);
    if (num_matched != 1) {
      fprintf(stderr,
          "Warning: Bad value \"%s\" in ACR_GEN_THREADS environment"
          " variable.\n"
          "         Default to %ld threads.\n", codegen_env, num_threads);
      *codegen = (size_t) num_threads;
    } else {
      env_threads = env_threads < 0 ? -env_threads : env_threads;
      *codegen = (size_t) env_threads;
      *codegen = *codegen == 0 ? 1 : *codegen;
    }
  }
  char *compile_env = getenv("ACR_COMPILE_THREADS");
  if (compile_env == NULL) {
    *compile = (size_t) num_threads;
  } else {
    long env_threads;
    int num_matched = sscanf(compile_env, "%ld", &env_threads);
    if (num_matched != 1) {
      fprintf(stderr,
          "Warning: Bad value \"%s\" in ACR_COMPILE_THREADS environment"
          " variable.\n"
          "         Default to %ld threads.\n", compile_env, num_threads);
      *compile = (size_t) num_threads;
    } else {
      env_threads = env_threads < 0 ? -env_threads : env_threads;
      *compile = (size_t) env_threads;
      *compile = *compile == 0 ? 1 : *compile;
    }
  }
}

void init_acr_runtime_data(
    struct acr_runtime_data* data,
    char *scop,
    size_t scop_size) {
  init_num_threads(&data->num_codegen_threads, &data->num_compile_threads);
  data->osl_relation = acr_read_scop_from_buffer(scop, scop_size);

  data->monitor_total_size = 1;
  for (size_t i = 0; i < data->num_monitor_dims; ++i) {
    data->monitor_total_size *= data->monitor_dim_max[i];
  }
  data->usability_inital_value = 3;

  for (size_t j = 0; j < data->num_alternatives; ++j) {
    struct runtime_alternative *alt = &data->alternatives[j];
    alt->restricted_domains =
      malloc(data->num_codegen_threads * sizeof(*alt->restricted_domains));
  }
  CloogInput **cloog_inputs =
    malloc(data->num_codegen_threads * sizeof(*cloog_inputs));
  data->context =
    malloc(data->num_codegen_threads * sizeof(*data->context));
  data->state =
    malloc(data->num_codegen_threads * sizeof(*data->state));
  data->statement_maps =
    malloc(data->num_codegen_threads * sizeof(*data->statement_maps));

  for (size_t i = 0; i < data->num_codegen_threads; ++i) {
    data->state[i] = cloog_state_malloc();
    cloog_inputs[i] = cloog_input_from_osl_scop(data->state[i],
      data->osl_relation);
    data->context[i] = isl_set_from_cloog_domain(cloog_inputs[i]->context);
    data->statement_maps[i] =
      malloc(data->num_statements * sizeof(**data->statement_maps));
    CloogNamedDomainList *domain_list = cloog_inputs[i]->ud->domain;
    for (size_t j = 0; j < data->num_statements; ++j, domain_list = domain_list->next) {
      data->statement_maps[i][j] = isl_map_copy(
          isl_map_from_cloog_scattering(domain_list->scattering));
    }
    for (size_t j = 0; j < data->num_alternatives; ++j) {
      struct runtime_alternative *alt = &data->alternatives[j];
      alt->restricted_domains[i] =
        malloc(data->num_statements * sizeof(*alt->restricted_domains[i]));
      domain_list = cloog_inputs[i]->ud->domain;
      for(size_t k = 0; k < data->num_statements; ++k, domain_list = domain_list->next) {
        alt->restricted_domains[i][k] =
          isl_set_copy(isl_set_from_cloog_domain(domain_list->domain));
      }
    }
    cloog_union_domain_free(cloog_inputs[i]->ud);
    free(cloog_inputs[i]);
  }
  acr_gencode_init_scop_to_match_alternatives(data);
  free(cloog_inputs);
  init_isl_tiling_domain(data);
  init_compile_flags(data);
}

unsigned char* acr_runtime_get_runtime_data(struct acr_runtime_data* data) {
  return data->current_monitoring_data;
}

size_t acr_runtime_get_grid_size(struct acr_runtime_data* data) {
  return data->grid_size;
}

void init_acr_static_data(
    struct acr_runtime_data_static *static_data,
    char *scop,
    size_t scop_size) {
  static_data->scop = acr_read_scop_from_buffer(scop, scop_size);
  static_data->state = cloog_state_malloc();
  CloogInput *cloog_inputs = cloog_input_from_osl_scop(static_data->state, static_data->scop);
  static_data->union_domain = cloog_inputs->ud;
  static_data->context = cloog_inputs->context;
  free(cloog_inputs);
}

void free_acr_static_data(struct acr_runtime_data_static *static_data) {
  osl_scop_free(static_data->scop);
}

void acr_static_data_init_grid(struct acr_runtime_data_static *static_data) {
  const size_t num_alternatives = static_data->num_alternatives;
  static_data->functions =
    malloc(num_alternatives * sizeof(*static_data->functions));
  /*acr_static_data_init_lexicographic_min_max(static_data);*/


  char *tile_library_c_code;
  size_t library_size;
  acr_code_generation_generate_tiling_library(
      static_data, &library_size, &tile_library_c_code);

  fprintf(stderr, "LIBRARY CODE #####\n%s\nEND LIBRARY CODE ####\n", tile_library_c_code);

  free(tile_library_c_code);
  exit(1);
}
