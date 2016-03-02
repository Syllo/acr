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

static osl_scop_p acr_construct_scop_with_only_monitor_iterators(
    const acr_option monitor,
    const osl_scop_p scop,
    const osl_statement_p statement) {
  acr_array_declaration *array_decl = acr_monitor_get_array_declaration(monitor);
  acr_array_dimensions_list dim_list =
    acr_array_decl_get_dimensions_list(array_decl);
  osl_scop_p new_scop = osl_scop_malloc();
  new_scop->version = scop->version;
  new_scop->language = acr_strdup(scop->language);
  osl_scop_register_extension(new_scop, osl_scatnames_interface());
  osl_scop_register_extension(new_scop, osl_body_interface());
  osl_scop_register_extension(new_scop, osl_strings_interface());
  osl_relation_add(&new_scop->context, osl_relation_clone(scop->context));
  osl_generic_add(&new_scop->parameters, osl_generic_clone(scop->parameters));
  osl_scop_print(stderr, new_scop);
  return NULL;
}

static void osl_set_scattering_base(
    unsigned long num_iterators, unsigned long num_parameters,
    osl_relation_p scattering) {
  osl_relation_print(stderr, scattering);
  for (unsigned int i = 0; i < num_iterators; ++i) {
    unsigned long iterator_row = i * 2 + 1;
    unsigned long iterator_column = 1 + i * 2 + 1 + i;
    unsigned long beta_row = i * 2;
    unsigned long beta_column = i;
    fprintf(stderr, "beta row %lu beta column%lu\n", beta_row, beta_column);
    osl_int_decrement(scattering->precision, &scattering->m[beta_row][beta_column],
        scattering->m[beta_row][beta_column]);
  osl_relation_print(stderr, scattering);
  }
  osl_relation_print(stderr, scattering);
}

static osl_relation_p acr_openscop_new_scattering_relation(
    unsigned long num_iterators, unsigned long num_parameters) {
  osl_relation_p scattering = osl_relation_malloc(
      num_iterators*2+1,
      3 + num_parameters + num_iterators * 3);
  osl_relation_set_attributes(scattering, num_iterators*2+1, num_iterators, 0, num_parameters);
  osl_relation_print(stderr, scattering);
  osl_set_scattering_base(
      num_iterators, num_parameters, scattering);
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

osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const osl_scop_p scop) {
  osl_strings_p parameters = acr_openscop_get_monitor_parameters(monitor);
  osl_strings_print(stdout, parameters);
  acr_openscop_new_scattering_relation(2,2);
  return NULL;
}
