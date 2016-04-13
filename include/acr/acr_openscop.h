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

#ifndef __ACR_OPENSCOP_H
#define __ACR_OPENSCOP_H

#include "acr/pragma_struct.h"

#include <osl/scop.h>
#include <isl/set.h>

enum acr_dimension_type {
  acr_dimension_type_bound_to_alternative,
  acr_dimension_type_bound_to_monitor,
  acr_dimension_type_free_dim,
};

typedef struct dimensions_upper_lower_bounds {
  size_t num_dimensions;
  size_t num_parameters;
  bool **lower_bound;
  bool **upper_bound;
  isl_set *bound_lexmin;
  isl_set *bound_lexmax;
  enum acr_dimension_type *dimensions_type;
  bool **has_constraint_with_previous_dim;
} dimensions_upper_lower_bounds;

typedef struct dimensions_upper_lower_bounds_all_statements {
  size_t num_statements;
  dimensions_upper_lower_bounds **statements_bounds;
} dimensions_upper_lower_bounds_all_statements;

void acr_print_scop_to_buffer(osl_scop_p scop, char** buffer,
    size_t* size_buffer);

osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const osl_scop_p scop,
    size_t grid_size,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used,
    const char *prefix);

osl_strings_p acr_openscop_get_monitor_parameters(const acr_option monitor);

void osl_relation_swap_column(osl_relation_p relation,
    int pos1,
    int pos2);

void acr_openscop_set_tiled_to_do_min_max(
    const acr_option monitor,
    const char* filter_function,
    size_t grid_size,
    bool max,
    const size_t *monitor_dim,
    osl_scop_p scop,
    const char *prefix);

void acr_openscop_set_tiled_to_do_avg(
    const acr_option monitor,
    const char* filter_function,
    size_t grid_size,
    const size_t *monitor_dim,
    osl_scop_p scop,
    const char *prefix);

dimensions_upper_lower_bounds_all_statements* acr_osl_get_upper_lower_bound_all(
        const osl_statement_p statement_list);

dimensions_upper_lower_bounds* acr_osl_get_upper_lower_bound_statement(
    const osl_statement_p statement);

void acr_osl_free_dimension_upper_lower_bounds(
    dimensions_upper_lower_bounds *bounds);

void acr_osl_free_dimension_upper_lower_bounds_all(
    dimensions_upper_lower_bounds_all_statements *bounds_all);

bool acr_osl_find_and_verify_free_dims_position(
    const acr_compute_node node,
    const osl_scop_p scop,
    dimensions_upper_lower_bounds_all_statements *bounds_all);

bool acr_osl_dim_has_constraints_with_dim(
    size_t dim1,
    size_t dim2,
    const dimensions_upper_lower_bounds *bounds);

bool acr_osl_dim_has_constraints_with_previous_dims(
    size_t dim1,
    const dimensions_upper_lower_bounds *bounds);

osl_strings_p acr_openscop_get_monitor_identifiers(const acr_option monitor);

bool acr_osl_check_if_identifiers_are_not_parameters(const osl_scop_p scop,
    osl_strings_p identifiers);

void acr_openscop_get_identifiers_with_dependencies(
    const acr_option monitor,
    const osl_scop_p scop,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used,
    osl_strings_p *all_iterators,
    osl_statement_p *statement_containing_them);

dimensions_upper_lower_bounds* acr_osl_get_min_max_bound_statement(
    const osl_statement_p statement);

osl_strings_p acr_osl_get_alternative_parameters(
    const acr_compute_node node);

#endif // __ACR_OPENSCOP_H
