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

#include "acr/print.h"

void pprint_acr_compute_node(FILE* out, acr_compute_node node) {
  unsigned long int size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list list = acr_compute_node_get_option_list(node);
  for (unsigned long int i = 0; i < size_list; ++i) {
    pprint_acr_option(out, acr_option_list_get_option(i, list));
  }
}

void pprint_acr_option(FILE* out, acr_option option) {
  switch (acr_get_type(option)) {
    case acr_type_alternative:
      pprint_acr_alternative(out, option);
      break;
    case acr_type_destroy:
      pprint_acr_destroy(out, option);
      break;
    case acr_type_grid:
      pprint_acr_grid(out, option);
      break;
    case acr_type_init:
      pprint_acr_init(out, option);
      break;
    case acr_type_monitor:
      pprint_acr_monitor(out, option);
      break;
    case acr_type_strategy:
      pprint_acr_strategy(out, option);
      break;
    case acr_type_unknown:
      break;
  }
}


void pprint_acr_alternative(FILE* out, acr_option option) {
  fprintf(out, "|---| ALTERNATIVE: %s\n",
      acr_alternative_get_alternative_name(option));
  switch (acr_alternative_get_type(option)) {
    case acr_alternative_parameter:
      fprintf(out, "|   |---| Parameter\n");
      fprintf(out, "|       | %s -> %ld\n",
          acr_alternative_get_object_to_swap_name(option),
          acr_alternative_get_replacement_parameter(option));
      break;
    case acr_alternative_function:
      fprintf(out, "|   |---| Function\n");
      fprintf(out, "|       | %s -> %s\n",
          acr_alternative_get_object_to_swap_name(option),
          acr_alternative_get_replacement_function(option));
      break;
    case acr_alternative_unknown:
      break;
  }
  fprintf(out, "|\n");
}

void pprint_acr_destroy(FILE* out, acr_option destroy) {
  fprintf(out, "|---| DESTROY\n");
  fprintf(out, "|   |---| Position: %lu\n", acr_destroy_get_row_position(destroy));
  fprintf(out, "|\n");
}

void pprint_acr_grid(FILE* out, acr_option grid) {
  fprintf(out, "|---| GRID\n");
  fprintf(out, "|   |---| Grid size: %lu\n", acr_grid_get_grid_size(grid));
  fprintf(out, "|\n");
}

void pprint_acr_init(FILE* out, acr_option init) {
  fprintf(out, "|---| INIT\n");
  fprintf(out, "|   |---| Position: %lu\n", acr_init_get_pragma_row_position(init));
  fprintf(out, "|   |---| Function: void %s(",
      acr_init_get_function_name(init));
  pprint_acr_parameter_declaration_list(out, acr_init_get_num_parameters(init),
      acr_init_get_parameter_list(init));
  fprintf(out, ")\n|\n");
}

void pprint_acr_monitor(FILE* out, acr_option monitor) {
  fprintf(out, "|---| MONITOR\n");
  fprintf(out, "|   |---| Data:\n");
  fprintf(out, "|   |   | ");
  pprint_acr_array_declaration(out, acr_monitor_get_array_declaration(monitor));
  fprintf(out, "\n|   |---| Monitoring function:\n");
  switch (acr_monitor_get_function(monitor)) {
    case acr_monitor_function_min:
      fprintf(out, "|   |   | min\n");
      break;
    case acr_monitor_function_max:
      fprintf(out, "|   |   | max\n");
      break;
    case acr_monitor_function_unknown:
      break;
  }
  char* filter_function = acr_monitor_get_filter_name(monitor);
  if (filter_function) {
    fprintf(out, "|   |---| Filter function:\n");
    fprintf(out, "|   |   | %s\n", filter_function);
  }
  fprintf(out, "|\n");
}

void pprint_acr_strategy(FILE* out, acr_option strategy) {
  long int strategy_val_integer[2];
  float strategy_val_floating_point[2];
  acr_strategy_populate_int_val(strategy, strategy_val_integer);
  acr_strategy_populate_float_val(strategy, strategy_val_floating_point);
  fprintf(out, "|---| STRATEGY\n");
  switch (acr_strategy_get_strategy_type(strategy)) {
    case acr_strategy_direct:
      fprintf(out, "|   |---| Direct\n");
      fprintf(out, "|   |   | %s <- ",
          acr_strategy_get_name(strategy));
      switch (acr_strategy_get_value_type(strategy)) {
        case acr_strategy_integer:
          fprintf(out, "%ld\n", strategy_val_integer[0]);
          break;
        case acr_strategy_floating_point:
          fprintf(out, "%f\n", strategy_val_floating_point[0]);
          break;
      }
      break;
    case acr_strategy_range:
      fprintf(out, "|   |---| Range\n");
      fprintf(out, "|   |   | %s <- ",
          acr_strategy_get_name(strategy));
      switch (acr_strategy_get_value_type(strategy)) {
        case acr_strategy_integer:
          fprintf(out, "[%ld , %ld]\n", strategy_val_integer[0],
              strategy_val_integer[1]);
          break;
        case acr_strategy_floating_point:
          fprintf(out, "[%f , %f]\n", strategy_val_floating_point[0],
              strategy_val_floating_point[1]);
          break;
      }
      break;
    case acr_strategy_unknown:
      break;
  }
  fprintf(out, "|\n");
}

void pprint_acr_parameter_declaration_list(FILE* out,
                                      unsigned long int num_declarations,
                                  acr_parameter_declaration* declaration_list) {
  if (num_declarations > 0) {
    for(unsigned long int i = 0; i < num_declarations - 1; ++i) {
      pprint_acr_parameter_specifier_list(out,
          acr_parameter_declaration_get_num_specifiers(declaration_list, i),
          acr_parameter_declaration_get_specif_list(declaration_list, i));
      fprintf(out, "%s, ",
          acr_parameter_declaration_get_parameter_name(declaration_list, i));
    }
    pprint_acr_parameter_specifier_list(out,
        acr_parameter_declaration_get_num_specifiers(declaration_list,
          num_declarations - 1),
        acr_parameter_declaration_get_specif_list(declaration_list,
          num_declarations - 1));
    fprintf(out, "%s",
        acr_parameter_declaration_get_parameter_name(declaration_list,
          num_declarations - 1));
  }
}

void pprint_acr_parameter_specifier_list(FILE* out,
                                      unsigned long int num_specifiers,
                                      acr_parameter_specifier* specifier_list) {
  for(unsigned long int i = 0; i < num_specifiers; ++i) {
    fprintf(out, "%s", acr_parameter_specifier_get_specifier(specifier_list, i));
    unsigned long int pointer_depth =
      acr_parameter_specifier_get_pointer_depth(specifier_list, i);
    for(unsigned long int j = 0; j < pointer_depth; ++j) {
      fprintf(out, "*");
    }
    fprintf(out, " ");
  }
}

void pprint_acr_array_declaration(FILE* out,
                                  acr_array_declaration* declaration) {
  pprint_acr_parameter_specifier_list(out,
      acr_array_decl_get_num_specifiers(declaration),
      acr_array_decl_get_specifiers_list(declaration));
  fprintf(out, " %s", acr_array_decl_get_array_name(declaration));
  unsigned long int num_dimensions = acr_array_decl_get_num_dimensions(declaration);
  unsigned long int* dimensions = acr_array_decl_get_dimensions_array(declaration);
  for (unsigned long int i = 0; i < num_dimensions; ++i) {
    fprintf(out, "[%lu]", dimensions[i]);
  }
}
