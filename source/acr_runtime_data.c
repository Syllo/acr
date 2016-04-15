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

#include <cloog/isl/domain.h>
#include <isl/set.h>
#include <isl/map.h>
#include <pthread.h>

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
  free(data->statement_maps);
  isl_set_free(data->context);
  cloog_state_free(data->state);
  data->state = NULL;
  osl_scop_free(data->osl_relation);
  data->osl_relation = NULL;
  pthread_spin_destroy(&data->alternative_lock);
  free(data->monitor_dim_max);
}

isl_map* isl_map_from_cloog_scattering(CloogScattering *scat);

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
}
