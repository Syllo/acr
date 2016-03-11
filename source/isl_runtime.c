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

#include "acr/isl_runtime.h"

isl_set* acr_isl_set_from_monitor(
    acr_monitored_data *data,
    unsigned long num_strategies,
    unsigned long int num_param,
    unsigned long num_dimensions,
    size_t *dimensions,
    size_t dimensions_total_size,
    unsigned long tiling_size,
    struct runtime_alternative*
        (*get_alternative_from_val)(acr_monitored_data data)) {

  /*isl_ctx *ctx = isl_ctx_alloc();*/
  /*isl_space *space = isl_space_set_alloc(ctx, num_param, num_dimensions);*/

  /*isl_basic_set **sets = malloc(num_strategies * sizeof(*sets));*/
  /*for (size_t i = 0; i < num_strategies; ++i) {*/
    /*sets[i] = isl_basic_set_empty(isl_space_copy(space));*/
  /*}*/

  size_t *current_dimension = calloc(num_dimensions, sizeof(*current_dimension));
  for(size_t i = 0; i < dimensions_total_size; ++i) {
    fprintf(stderr, "Data %lu : %d\n", i, data[i]);
    struct runtime_alternative *alternative = get_alternative_from_val(data[i]);
    assert(alternative != NULL);
    for (size_t j = 0; j < num_dimensions - 1; ++j) {
      current_dimension[j] +=1;
      if(current_dimension[j] == dimensions[j]) {
        current_dimension[j] = 0;
        current_dimension[j+1] += 1;
      } else {
        break;
      }
    }
  }
  free(current_dimension);
}
