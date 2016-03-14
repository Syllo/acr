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

#include <isl/constraint.h>
#include <isl/ctx.h>
#include <isl/space.h>
#include <isl/val.h>

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

  isl_ctx *ctx = isl_ctx_alloc();
  isl_space *space = isl_space_set_alloc(ctx, num_param, num_dimensions);
  isl_val *tiling_size_val = isl_val_int_from_ui(ctx, tiling_size);

  isl_basic_set **sets = malloc(dimensions_total_size * sizeof(*sets));
  for (size_t i = 0; i < dimensions_total_size; ++i) {
    sets[i] = isl_basic_set_universe(isl_space_copy(space));
  }
  isl_local_space *local_space = isl_local_space_from_space(space);

  unsigned long *current_dimension = malloc(num_dimensions * sizeof(*current_dimension));
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    current_dimension[i] = 0ul;
  }
  for(size_t i = 0; i < dimensions_total_size; ++i) {
    /*fprintf(stderr, "Data %lu : %d\n", i, data[i]);*/
    struct runtime_alternative *alternative = get_alternative_from_val(data[i]);
    assert(alternative != NULL);

    for (unsigned long j = 0; j < num_dimensions; ++j) {
      unsigned long dimensions_pos = current_dimension[j];
      fprintf(stderr, "current dimension %lu\n", dimensions_pos);
      isl_constraint *c_lower = isl_constraint_alloc_inequality(
          isl_local_space_copy(local_space));
      isl_constraint *c_upper = isl_constraint_copy(c_lower);
      isl_val *lower_bound =
        isl_val_mul_ui(isl_val_copy(tiling_size_val), 3);
      lower_bound = isl_val_neg(lower_bound);
      isl_val *upper_bound =
        isl_val_mul_ui(isl_val_copy(tiling_size_val), 1);
      upper_bound =
        isl_val_mul_ui(upper_bound, dimensions_pos);
      upper_bound = isl_val_add(upper_bound, isl_val_copy(tiling_size_val));
      upper_bound = isl_val_sub_ui(upper_bound, 1ul);
      fprintf(stderr, "Lower bound: %ld\n", isl_val_get_num_si(lower_bound));
      c_lower =
        isl_constraint_set_constant_val(c_lower, lower_bound);
      c_lower = isl_constraint_set_coefficient_si(c_lower, isl_dim_set, j, 1);
      fprintf(stderr, "Upper bound: %ld\n", isl_val_get_num_si(upper_bound));
      c_upper =
        isl_constraint_set_constant_val(c_upper, upper_bound);
      c_upper = isl_constraint_set_coefficient_si(c_upper, isl_dim_set, j, -1);
      /*sets[j] = isl_basic_set_add_constraint(sets[j], c_lower);*/
      isl_basic_set_print_internal(sets[i], stderr, 0);
      sets[i] = isl_basic_set_add_constraint(sets[j], c_upper);
      isl_basic_set_print_internal(sets[i], stderr, 0);
    }


    fprintf(stderr, "Dimension: ");
    for (size_t j = 0; j < num_dimensions; ++j) {
      fprintf(stderr, "%lu ", current_dimension[j]);
    }
    fprintf(stderr, "\n");

    for (size_t j = 0; j < num_dimensions; ++j) {
      current_dimension[j] += 1;
      if(current_dimension[j] == dimensions[j]) {
        current_dimension[j] = 0;
      } else {
        break;
      }
    }
  }
  free(current_dimension);
}
