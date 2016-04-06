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

#include <isl/set.h>
#include <pthread.h>

void free_acr_runtime_data(struct acr_runtime_data* data) {
  for (unsigned long i = 0; i < data->num_alternatives; ++i) {
    struct runtime_alternative *alt = &data->alternatives[i];
    if (alt->type == acr_runtime_alternative_parameter) {
      for (unsigned long j = 0; j < alt->value.alt.parameter.num_domains; ++j) {
        isl_set_free(alt->value.alt.parameter.parameter_constraints[j]);
      }
      free(alt->value.alt.parameter.parameter_constraints);
    }
  }
  cloog_union_domain_free(data->cloog_input->ud);
  cloog_domain_free(data->cloog_input->context);
  free(data->cloog_input);
  data->cloog_input = NULL;
  cloog_state_free(data->state);
  data->state = NULL;
  osl_scop_free(data->osl_relation);
  data->osl_relation = NULL;
  data->monitor_thread_continue = false;
  pthread_join(data->monitor_thread, NULL);
  pthread_spin_destroy(&data->alternative_lock);
}

void init_acr_runtime_data(
    struct acr_runtime_data* data,
    char *scop,
    size_t scop_size) {
  data->osl_relation = acr_read_scop_from_buffer(scop, scop_size);
  data->state = cloog_state_malloc();
  data->cloog_input = cloog_input_from_osl_scop(data->state,
      data->osl_relation);
  acr_cloog_init_scop_to_match_alternatives(data);
  data->monitor_total_size = 1;
  for (unsigned long i = 0; i < data->num_monitor_dims; ++i) {
    data->monitor_total_size *= data->monitor_dim_max[i];
  }
  pthread_spin_init(&data->alternative_lock, PTHREAD_PROCESS_PRIVATE);
  data->usability_inital_value = 3;
}
