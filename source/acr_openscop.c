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

#include "acr/acr_openscop.h"

#include <osl/body.h>
#include <osl/extensions/scatnames.h>
#include <osl/macros.h>
#include <stdio.h>

void acr_print_scop_to_buffer(osl_scop_p scop, char** buffer,
    size_t* size_buffer) {
  FILE* output_char = open_memstream(buffer, size_buffer);
  osl_scop_print(output_char, scop);
  fclose(output_char);
}

osl_scop_p acr_read_scop_from_buffer(char* buffer, size_t size_buffer) {
  FILE* fakefile = fmemopen(buffer, size_buffer, "r");
  osl_scop_p scop = osl_scop_read(fakefile);
  fclose(fakefile);
  return scop;
}

/*static osl_scop_p acr_construct_scop_with_only_monitor_iterators(*/
    /*const acr_option monitor,*/
    /*const osl_scop_p scop,*/
    /*const osl_statement_p statement) {*/
  /*acr_array_declaration *array_decl = acr_monitor_get_array_declaration(monitor);*/
  /*acr_array_dimensions_list dim_list =*/
    /*acr_array_decl_get_dimensions_list(array_decl);*/
  /*osl_scop_p new_scop = osl_scop_malloc();*/
  /*new_scop->version = scop->version;*/
  /*new_scop->language = acr_strdup(scop->language);*/
  /*osl_scop_register_extension(new_scop, osl_scatnames_interface());*/
  /*osl_scop_register_extension(new_scop, osl_body_interface());*/
  /*osl_scop_register_extension(new_scop, osl_strings_interface());*/
  /*osl_relation_add(&new_scop->context, osl_relation_clone(scop->context));*/
  /*osl_generic_add(&new_scop->parameters, osl_generic_clone(scop->parameters));*/
  /*osl_scop_print(stderr, new_scop);*/
  /*return NULL;*/
/*}*/

static void osl_set_scattering_base(
    unsigned long num_iterators,
    osl_relation_p scattering) {
  // update beta
  for (unsigned int i = 0; i < num_iterators; ++i) {
    unsigned long iterator_column = 2 + num_iterators * 2 + i;
    unsigned long scatter_row = i * 2 + 1;
    unsigned long scatter_column = 2 + i * 2;
    unsigned long beta_row = i * 2;
    unsigned long beta_column = 1 + i * 2;
    osl_int_decrement(scattering->precision, &scattering->m[beta_row][beta_column],
        scattering->m[beta_row][beta_column]);
    osl_int_decrement(scattering->precision,
        &scattering->m[scatter_row][scatter_column],
        scattering->m[scatter_row][scatter_column]);
    osl_int_increment(scattering->precision,
        &scattering->m[scatter_row][iterator_column],
        scattering->m[scatter_row][iterator_column]);
  }
  osl_int_decrement(scattering->precision,
      &scattering->m[num_iterators*2][num_iterators*2+1],
      scattering->m[num_iterators*2][num_iterators*2+1]);
}

void osl_relation_swap_column(osl_relation_p relation,
    int pos1,
    int pos2) {
  if (relation == NULL || pos1 > relation->nb_columns || pos2 > relation->nb_columns)
    return;

  osl_int_p temp = osl_int_malloc(relation->precision);

  for (int i = 0; i < relation->nb_rows; ++i) {
    osl_int_assign(relation->precision,
        temp, relation->m[i][pos1]);
    osl_int_assign(relation->precision,
        &relation->m[i][pos1], relation->m[i][pos2]);
    osl_int_assign(relation->precision,
       &relation->m[i][pos2], *temp);
  }

  osl_int_free(relation->precision, temp);
}

static void acr_osl_apply_tiling(
    unsigned long num_iterators,
    osl_relation_p scattering,
    unsigned long tiling_size) {
  osl_int_p temp_val = osl_int_malloc(scattering->precision);
  int last_row = scattering->nb_rows - 1;
  for (unsigned long i = 0; i < num_iterators; ++i) {
    int where_to_add_two_column = 2 + i * 4;
    osl_relation_insert_blank_column(scattering, where_to_add_two_column);

    // Set beta to zero
    osl_relation_insert_blank_row(scattering, -1);
    last_row += 1;
    osl_int_decrement(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column],
        scattering->m[last_row][where_to_add_two_column]);

    osl_relation_insert_blank_column(scattering, where_to_add_two_column);
    // Set new scatter to do strip-mine
    osl_relation_insert_blank_row(scattering, -1);
    last_row += 1;
    osl_int_set_si(scattering->precision, temp_val, tiling_size);
    osl_int_assign(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column], *temp_val);
    osl_int_assign(scattering->precision,
        &scattering->m[last_row][scattering->nb_columns-1],
        *temp_val);
    osl_int_decrement(scattering->precision,
        &scattering->m[last_row][scattering->nb_columns-1],
        scattering->m[last_row][scattering->nb_columns-1]);
    osl_int_increment(scattering->precision,
        &scattering->m[last_row][0],
        scattering->m[last_row][0]);
    osl_int_decrement(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column + 2],
        scattering->m[last_row][where_to_add_two_column + 2]);

    osl_relation_insert_blank_row(scattering, -1);
    last_row += 1;
    osl_int_oppose(scattering->precision,
        temp_val, *temp_val);
    osl_int_assign(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column], *temp_val);
    osl_int_increment(scattering->precision,
        &scattering->m[last_row][0],
        scattering->m[last_row][0]);
    osl_int_increment(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column + 2],
        scattering->m[last_row][where_to_add_two_column + 2]);

    scattering->nb_output_dims += 2;

    for (int j = i; j > 0; --j) {
      osl_relation_swap_column(scattering, where_to_add_two_column, where_to_add_two_column - 2);
      where_to_add_two_column -= 2;
    }
  }
  osl_int_free(scattering->precision, temp_val);
}

static osl_relation_p acr_openscop_new_scattering_relation(
    unsigned long num_iterators, unsigned long num_parameters) {
  osl_relation_p scattering = osl_relation_malloc(
      num_iterators*2+1,
      3 + num_parameters + num_iterators * 3);
  osl_relation_set_type(scattering, OSL_TYPE_SCATTERING);
  osl_relation_set_attributes(scattering, num_iterators*2+1, num_iterators, 0, num_parameters);
  osl_set_scattering_base(num_iterators, scattering);
  return scattering;
}

static void _acr_openscop_add_missing_parameters(unsigned long* size_string,
    osl_strings_p parameters,
    const acr_array_dimension dimension) {
  if (dimension->type != acr_array_dim_leaf) {
    _acr_openscop_add_missing_parameters(size_string, parameters,
        dimension->val.node.left);
    _acr_openscop_add_missing_parameters(size_string, parameters,
        dimension->val.node.right);
  } else {
    switch (dimension->val.leaf.type) {
      case acr_expr_leaf_int:
        break;
      case acr_expr_leaf_param:
        if (osl_strings_find(parameters, dimension->val.leaf.value.parameter)
            == *size_string) {
          osl_strings_add(parameters, dimension->val.leaf.value.parameter);
          *size_string += 1;
        }
        break;
    }
  }
}

osl_strings_p acr_openscop_get_monitor_parameters(const acr_option monitor) {
  size_t string_size = 0;
  osl_strings_p parameters = osl_strings_malloc();
  acr_array_declaration* array_decl =
    acr_monitor_get_array_declaration(monitor);
  acr_array_dimensions_list dim_list = array_decl->array_dimensions_list;
  for (unsigned long i = 0; i < array_decl->num_dimensions; ++i) {
    _acr_openscop_add_missing_parameters(&string_size, parameters, dim_list[i]);
  }
  return parameters;
}

unsigned long acr_monitor_num_dimensions(const acr_option monitor) {
  return monitor->options.monitor.data_monitored.num_dimensions;
}

static void acr_osl_update_lower_bound_from_dimension(
    osl_relation_p relation,
    unsigned long which_dim) {
  osl_int_increment(relation->precision,
      &relation->m[which_dim*2][0],
      relation->m[which_dim*2][0]);
  osl_int_increment(relation->precision,
      &relation->m[which_dim*2][which_dim + 1],
      relation->m[which_dim*2][which_dim + 1]);
}

static bool acr_osl_verify_affinity(const acr_array_dimension dim) {
  if(dim->type != acr_array_dim_leaf) {
    switch (dim->type) {
      case acr_array_dim_mul:
      case acr_array_dim_div:
        if (acr_osl_verify_affinity(dim->val.node.left) &&
            acr_osl_verify_affinity(dim->val.node.right))
        break;
      default:
        return false;
    }
  } else {
    switch (dim->val.leaf.type) {
      case acr_expr_leaf_param:
        return true;
      case acr_expr_leaf_int:
        return false;
    }
  }
}

static void acr_osl_update_upper_bound_from_dimension(
    osl_relation_p relation,
    unsigned long which_dim,
    const acr_array_dimension dim,
    const osl_strings_p parameters) {
  osl_int_increment(relation->precision,
      &relation->m[which_dim*2+1][0],
      relation->m[which_dim*2+1][0]);
  if (dim->type != acr_array_dim_leaf) {
    if (acr_osl_verify_affinity(dim->val.node.left))
      return;
    if (acr_osl_verify_affinity(dim->val.node.right))
      return;
    
  }
}

osl_relation_p acr_openscop_domain_from_monitor(
    const acr_option monitor,
    size_t num_dimensions,
    size_t num_parameters,
    const osl_strings_p parameters) {
  osl_relation_p new_relation = osl_relation_malloc(num_dimensions * 2,
      num_dimensions + num_parameters + 2);
  osl_relation_set_type(new_relation, OSL_TYPE_DOMAIN);
  osl_relation_set_attributes(new_relation, num_dimensions, 0, 0, num_parameters);
  acr_array_declaration* decl = acr_monitor_get_array_declaration(monitor);
  acr_array_dimensions_list dim_list = acr_array_decl_get_dimensions_list(decl);
  for (size_t i = 0; i < num_dimensions; ++i) {
    acr_osl_update_lower_bound_from_dimension(
        new_relation, i);
    acr_osl_update_upper_bound_from_dimension(
        new_relation, i, dim_list[i], parameters);
  }
  return new_relation;
}

osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const osl_scop_p scop,
    unsigned long grid_size) {
  osl_strings_p parameters = acr_openscop_get_monitor_parameters(monitor);
  osl_strings_print(stdout, parameters);
  size_t num_param = osl_strings_size(parameters);
  size_t num_dimensions = acr_monitor_num_dimensions(monitor);
  osl_relation_p domain =
    acr_openscop_domain_from_monitor(monitor, num_dimensions, num_param,
        parameters);
  osl_relation_print(stderr, domain);
  osl_relation_p scattering =
    acr_openscop_new_scattering_relation(num_dimensions, num_param);
  osl_relation_print(stderr, scattering);
  acr_osl_apply_tiling(num_dimensions, scattering, grid_size);
  osl_relation_print(stderr, scattering);
  return NULL;
}
