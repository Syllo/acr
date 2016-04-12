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

#include <assert.h>

#include <isl/constraint.h>
#include <isl/ctx.h>
#include <isl/space.h>
#include <isl/val.h>

isl_set** acr_isl_set_from_monitor(
    isl_ctx *ctx,
    const unsigned char *data,
    size_t num_alternatives,
    unsigned int num_param,
    unsigned int num_dimensions,
    const size_t *dimensions,
    size_t dimensions_total_size,
    size_t tiling_size,
    struct runtime_alternative*
        (*get_alternative_from_val)(unsigned char data)) {

  isl_space *space = isl_space_set_alloc(ctx, num_param, num_dimensions);
  isl_val *tiling_size_val = isl_val_int_from_ui(ctx, tiling_size);

  isl_set **sets = malloc(num_alternatives * sizeof(*sets));
  for (size_t i = 0; i < num_alternatives; ++i) {
    sets[i] = isl_set_empty(isl_space_copy(space));
  }
  size_t *current_dimension =
    calloc(num_dimensions, sizeof(*current_dimension));
  isl_local_space *local_space =
    isl_local_space_from_space(isl_space_copy(space));

  for(size_t i = 0; i < dimensions_total_size; ++i) {
    struct runtime_alternative *alternative = get_alternative_from_val(data[i]);
    assert(alternative != NULL);

    isl_set *tempset = isl_set_universe(isl_space_copy(space));
    for (unsigned int j = 0; j < num_dimensions; ++j) {
      size_t dimensions_pos = current_dimension[num_dimensions - 1 - j];
      isl_constraint *c_lower = isl_constraint_alloc_inequality(
          isl_local_space_copy(local_space));
      isl_constraint *c_upper = isl_constraint_copy(c_lower);
      isl_val *lower_bound =
        isl_val_mul_ui(isl_val_copy(tiling_size_val), dimensions_pos);
      lower_bound = isl_val_neg(lower_bound);
      isl_val *upper_bound =
        isl_val_mul_ui(isl_val_copy(tiling_size_val), dimensions_pos);
      upper_bound = isl_val_add(upper_bound, isl_val_copy(tiling_size_val));
      upper_bound = isl_val_sub_ui(upper_bound, 1);
      c_lower =
        isl_constraint_set_constant_val(c_lower, lower_bound);
      c_lower = isl_constraint_set_coefficient_si(c_lower, isl_dim_set, (int)j, 1);
      c_upper =
        isl_constraint_set_constant_val(c_upper, upper_bound);
      c_upper = isl_constraint_set_coefficient_si(c_upper, isl_dim_set, (int)j, -1);
      tempset = isl_set_add_constraint(tempset, c_lower);
      tempset = isl_set_add_constraint(tempset, c_upper);
    }
    sets[alternative->alternative_number] =
      isl_set_union(sets[alternative->alternative_number], tempset);

    for (size_t j = 0; j < num_dimensions; ++j) {
      current_dimension[j] += 1;
      if(current_dimension[j] == dimensions[j] + 1) {
        current_dimension[j] = 0;
      } else {
        break;
      }
    }
  }

  isl_space_free(space);
  isl_local_space_free(local_space);
  isl_val_free(tiling_size_val);
  free(current_dimension);
  return sets;
}

isl_set* acr_isl_set_from_alternative_parameter_construct(
    isl_ctx *ctx,
    unsigned int num_parameters,
    unsigned int num_dimensions,
    struct runtime_alternative *alternative_list) {

  isl_val *constraint_value =
    isl_val_int_from_si(ctx,
        alternative_list->value.alt.parameter.parameter_value);
  isl_space *space = isl_space_set_alloc(ctx, num_parameters, num_dimensions);
  isl_set *new_set = isl_set_universe(isl_space_copy(space));
  isl_local_space *local_space = isl_local_space_from_space(space);
  isl_constraint *parameter_constraint = isl_constraint_alloc_equality(
      local_space);
  parameter_constraint =
    isl_constraint_set_constant_val(parameter_constraint, constraint_value);
  parameter_constraint =
    isl_constraint_set_coefficient_si(parameter_constraint, isl_dim_param,
        (int)alternative_list->value.alt.parameter.parameter_position,
        -1);
  new_set = isl_set_add_constraint(new_set, parameter_constraint);
  return new_set;
}
