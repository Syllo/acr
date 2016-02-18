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

#include <stdio.h>

#include <osl/extensions/coordinates.h>

void acr_get_start_and_stop_for_clan(const acr_compute_node node,
    unsigned long* start, unsigned long *stop) {

  *start = 0ul;
  *stop = 0ul;

  unsigned long option_list_size = acr_compute_node_get_option_list_size(node);
  const acr_option_list option_list = acr_compute_node_get_option_list(node);

  for (unsigned int i = 0ul; i < option_list_size; ++i) {
    const acr_option option = acr_option_list_get_option(i, option_list);
    switch (acr_option_get_type(option)) {
      case acr_type_init:
        *start = acr_init_get_pragma_position(option);
        *stop = *start;
        break;
      case acr_type_alternative:
        *start = acr_alternative_get_pragma_position(option);
        break;
      case acr_type_grid:
        *start = acr_grid_get_pragma_position(option);
        break;
      case acr_type_monitor:
        *start = acr_monitor_get_pragma_position(option);
        break;
      case acr_type_strategy:
        *start = acr_strategy_get_pragma_position(option);
        break;
      case acr_type_destroy:
        *stop = acr_destroy_get_pragma_position(option);
      case acr_type_unknown:
        break;
    }
  }
}

osl_scop_p acr_extract_scop_in_compute_node(const acr_compute_node node,
                                            FILE* input_file,
                                            const char* name_input_file) {

  clan_options_t clan_options;

  clan_options.autoscop = 1;
  clan_options.bounded_context = 0;
  clan_options.castle = 0;
  clan_options.extbody = 0;
  clan_options.inputscop = 0;
  clan_options.name = acr_strdup(name_input_file);
  clan_options.noloopcontext = 0;
  clan_options.nosimplify = 0;
  clan_options.outscoplib = 0;
  clan_options.precision = 0;
  clan_options.structure = 0;

  unsigned long start, end;
  acr_get_start_and_stop_for_clan(node, &start, &end);

  osl_scop_p scop = clan_scop_extract_delimited(input_file,
      &clan_options, start, end);
  free(clan_options.name);
  return scop;
}

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

void acr_scop_get_coordinates_start_end_kernel(
    const acr_compute_node compute_node,
    const osl_scop_p scop,
    unsigned long* start, unsigned long* end) {
  acr_get_start_and_stop_for_clan(compute_node, start, end);
  scop->extension->interface
}
