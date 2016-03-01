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
  osl_relation_add(&new_scop->context, osl_relation_clone(scop->context));
  osl_generic_add(&scop->parameters, osl_generic_clone(scop->parameters));
  osl_scop_register_extension(new_scop, osl_scatnames_interface());
  osl_scop_register_extension(new_scop, osl_body_interface());
  //osl_generic_p scatter_name = os

}

osl_scop_p acr_find_in_scop_monitor_loop(const acr_option monitor,
    const osl_scop_p scop) {
  osl_statement_p statement = scop->statement;
  acr_array_declaration *array_decl = acr_monitor_get_array_declaration(monitor);
  acr_array_dimensions_list dim_list =
    acr_array_decl_get_dimensions_list(array_decl);
  unsigned long list_size = acr_array_decl_get_num_dimensions(array_decl);
  while(statement != NULL) {
    for (unsigned long i = 0; i < list_size; ++i) {
      char* name = dim_list[i].value.parameter_name;
      osl_body_p body = osl_generic_lookup(statement->extension, OSL_URI_BODY);
      size_t string_size = osl_strings_size(body->iterators);
      if (osl_strings_find(body->iterators, name) < string_size) {
        if (i == list_size - 1) {
          return acr_construct_scop_with_only_monitor_iterators(monitor,
              scop, statement);
        }
      }
      else {
        break;
      }
    }
    statement = statement->next;
  }
  return NULL;
}
