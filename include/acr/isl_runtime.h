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

#ifndef __ACR_ISL_RUNTIME_H
#define __ACR_ISL_RUNTIME_H

#include <assert.h>
#include <isl/ctx.h>
#include <isl/set.h>
#include <isl/space.h>
#include <isl/union_set_type.h>

#include "acr/runtime_strategies.h"


isl_union_set* acr_isl_union_set_from_monitor(
    runtime_strategies strategies,
    acr_monitored_data data,
    unsigned long int num_param,
    unsigned long num_dimensions,
    size_t *dimensions,
    size_t dimensions_total_size,
    unsigned long tiling_size) {

  isl_ctx *ctx = isl_ctx_alloc();
  isl_space *space = isl_space_set_alloc(ctx, num_param, num_dimensions);

  isl_basic_set **sets = malloc(strategies->num_strategies * sizeof(*sets));
  for (size_t i = 0; i < strategies->num_strategies; ++i) {
    sets[i] = isl_basic_set_empty(isl_space_copy(space));
  }

  size_t *current_dimension = calloc(num_dimensions, sizeof(*current_dimension));
  for(size_t i = 0; i < dimensions_total_size; ++i) {
    runtime_alternative alternative = acr_runtime_get_alternative_from_val(
        i,
        data,
        strategies);
    assert(alternative);
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
}

#endif // __ACR_ISL_RUNTIME_H
