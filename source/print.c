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

#include <inttypes.h>
#include "acr/print.h"

static inline void pprint_acr_indent(FILE* out, size_t num) {
  for (size_t i = 0; i < num; ++i)
    fprintf(out, "|   ");
}

void pprint_acr_compute_node(FILE* out, acr_compute_node node,
    size_t indent_level) {
  if (!node) {
    pprint_acr_indent(out, indent_level);
    fprintf(out, "|---| Node\n|   | (NULL)\n|\n");
    return;
  }
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| Node\n");
  size_t size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list list = acr_compute_node_get_option_list(node);
  for (size_t i = 0; i < size_list; ++i) {
    pprint_acr_option(out, acr_option_list_get_option(i, list),
        indent_level + 1);
  }
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_option(FILE* out, acr_option option, size_t indent_level) {
  switch (acr_option_get_type(option)) {
    case acr_type_alternative:
      pprint_acr_alternative(out, option, indent_level);
      break;
    case acr_type_destroy:
      pprint_acr_destroy(out, option, indent_level);
      break;
    case acr_type_grid:
      pprint_acr_grid(out, option, indent_level);
      break;
    case acr_type_init:
      pprint_acr_init(out, option, indent_level);
      break;
    case acr_type_monitor:
      pprint_acr_monitor(out, option, indent_level);
      break;
    case acr_type_strategy:
      pprint_acr_strategy(out, option, indent_level);
      break;
    case acr_type_deferred_destroy:
      pprint_acr_deffered_destroy(out, option, indent_level);
      break;
    case acr_type_unknown:
      break;
  }
}

void pprint_acr_deffered_destroy(FILE *out, acr_option option,
    size_t indent_level) {
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| DEFERRED DESTROY\n");
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Position: %zu\n",
      acr_deferred_destroy_get_pragma_position(option));
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Referenced init: %s\n",
      acr_deferred_destroy_get_ref_init_name(option));
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_alternative(FILE* out, acr_option option, size_t indent_level) {
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| ALTERNATIVE: %s\n",
      acr_alternative_get_alternative_name(option));
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Position: %zu\n",
      acr_alternative_get_pragma_position(option));
  switch (acr_alternative_get_type(option)) {
    case acr_alternative_parameter:
      pprint_acr_indent(out, indent_level + 1);
      fprintf(out, "|---| Parameter\n");
      pprint_acr_indent(out, indent_level + 2);
      fprintf(out, "| %s -> %"PRIdMAX"\n",
          acr_alternative_get_object_to_swap_name(option),
          acr_alternative_get_replacement_parameter(option));
      break;
    case acr_alternative_function:
      pprint_acr_indent(out, indent_level + 1);
      fprintf(out, "|   |   |---| Function\n");
      pprint_acr_indent(out, indent_level + 2);
      fprintf(out, "|   |   |   | %s -> %s\n",
          acr_alternative_get_object_to_swap_name(option),
          acr_alternative_get_replacement_function(option));
      break;
    case acr_alternative_corner_computation:
      pprint_acr_indent(out, indent_level + 1);
      fprintf(out, "|   |   |---| Corner computation\n");
      break;
    case acr_alternative_zero_computation:
      pprint_acr_indent(out, indent_level + 1);
      fprintf(out, "|   |   |---| Zero computation\n");
      break;
    case acr_alternative_full_computation:
      pprint_acr_indent(out, indent_level + 1);
      fprintf(out, "|   |   |---| Full computation\n");
      break;
    case acr_alternative_unknown:
      break;
  }
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_destroy(FILE* out, acr_option destroy, size_t indent_level) {
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| DESTROY\n");
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Position: %zu\n",
      acr_destroy_get_pragma_position(destroy));
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_grid(FILE* out, acr_option grid, size_t indent_level) {
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| GRID\n");
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Position: %zu\n",
      acr_grid_get_pragma_position(grid));
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Grid size: %zu\n", acr_grid_get_grid_size(grid));
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_init(FILE* out, acr_option init, size_t indent_level) {
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| INIT\n");
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Position: %zu\n", acr_init_get_pragma_position(init));
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Function: void %s(",
      acr_init_get_function_name(init));
  pprint_acr_parameter_declaration_list(out, acr_init_get_num_parameters(init),
      acr_init_get_parameter_list(init));
  fprintf(out, ")\n");
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_monitor(FILE* out, acr_option monitor, size_t indent_level) {
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| MONITOR\n");
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Position: %zu\n",
      acr_monitor_get_pragma_position(monitor));
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Data:\n");
  pprint_acr_indent(out, indent_level + 2);
  fprintf(out, "| ");
  pprint_acr_array_declaration(out, acr_monitor_get_array_declaration(monitor));
  fprintf(out, "\n");
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Monitoring function:\n");
  pprint_acr_indent(out, indent_level + 2);
  switch (acr_monitor_get_function(monitor)) {
    case acr_monitor_function_min:
      fprintf(out, "| min\n");
      break;
    case acr_monitor_function_max:
      fprintf(out, "| max\n");
      break;
    case acr_monitor_function_avg:
      fprintf(out, "| avg\n");
      break;
    case acr_monitor_function_unknown:
      break;
  }
  char* filter_function = acr_monitor_get_filter_name(monitor);
  if (filter_function) {
    pprint_acr_indent(out, indent_level + 1);
    fprintf(out, "|---| Filter function:\n");
    pprint_acr_indent(out, indent_level + 2);
    fprintf(out, "| %s\n", filter_function);
  }
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_strategy(FILE* out, acr_option strategy, size_t indent_level) {
  intmax_t strategy_val_integer[2];
  float strategy_val_floating_point[2];
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|---| STRATEGY\n");
  pprint_acr_indent(out, indent_level + 1);
  fprintf(out, "|---| Position: %zu\n",
      acr_strategy_get_pragma_position(strategy));
  switch (acr_strategy_get_strategy_type(strategy)) {
    case acr_strategy_direct:
      pprint_acr_indent(out, indent_level + 1);
      fprintf(out, "|---| Direct\n");
      pprint_acr_indent(out, indent_level + 2);
      fprintf(out, "| %s <- ",
          acr_strategy_get_name(strategy));
      switch (acr_strategy_get_value_type(strategy)) {
        case acr_strategy_integer:
          acr_strategy_get_int_val(strategy, strategy_val_integer);
          fprintf(out, "%"PRIdMAX"\n", strategy_val_integer[0]);
          break;
        case acr_strategy_floating_point:
          acr_strategy_get_float_val(strategy, strategy_val_floating_point);
          fprintf(out, "%f\n", (double) strategy_val_floating_point[0]);
          break;
      }
      break;
    case acr_strategy_range:
      pprint_acr_indent(out, indent_level + 1);
      fprintf(out, "|---| Range\n");
      pprint_acr_indent(out, indent_level + 2);
      fprintf(out, "| %s <- ",
          acr_strategy_get_name(strategy));
      if (acr_strategy_get_value_type(strategy) == acr_strategy_integer) {
          acr_strategy_get_int_val(strategy, strategy_val_integer);
          fprintf(out, "[%"PRIdMAX" , %"PRIdMAX"]\n", strategy_val_integer[0],
              strategy_val_integer[1]);
      } else {
          acr_strategy_get_float_val(strategy, strategy_val_floating_point);
          fprintf(out, "[%f , %f]\n", (double) strategy_val_floating_point[0],
              (double) strategy_val_floating_point[1]);
      }
      break;
    case acr_strategy_unknown:
      break;
  }
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}

void pprint_acr_parameter_declaration_list(FILE* out,
                                      size_t num_declarations,
                                  acr_parameter_declaration* declaration_list) {
  if (num_declarations > 0) {
    for(size_t i = 0; i < num_declarations - 1; ++i) {
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
                                      size_t num_specifiers,
                                      acr_parameter_specifier* specifier_list) {
  for(size_t i = 0; i < num_specifiers; ++i) {
    fprintf(out, "%s", acr_parameter_specifier_get_specifier(specifier_list, i));
    size_t pointer_depth =
      acr_parameter_specifier_get_pointer_depth(specifier_list, i);
    for(size_t j = 0; j < pointer_depth; ++j) {
      fprintf(out, "*");
    }
    fprintf(out, " ");
  }
}

void print_acr_array_dimensions(FILE* out,
    acr_array_dimension dim, bool print_braces) {
  if (print_braces)
    fprintf(out, "[");
  fprintf(out, "%s", acr_array_dimension_get_identifier(dim));
  if (print_braces)
    fprintf(out, "]");
}

void pprint_acr_array_declaration(FILE* out,
                                  acr_array_declaration* declaration) {
  pprint_acr_parameter_specifier_list(out,
      acr_array_decl_get_num_specifiers(declaration),
      acr_array_decl_get_specifiers_list(declaration));
  fprintf(out, " %s", acr_array_decl_get_array_name(declaration));
  size_t num_dimensions = acr_array_decl_get_num_dimensions(declaration);
  acr_array_dimensions_list dimensions =
    acr_array_decl_get_dimensions_list(declaration);

  for (size_t i = 0; i < num_dimensions; ++i) {
    print_acr_array_dimensions(out, dimensions[i], true);
  }
}

void pprint_acr_compute_node_list(FILE* out,
                                  acr_compute_node_list node_list,
                                  size_t indent_level) {
  pprint_acr_indent(out, indent_level);
  for (size_t i = 0; i < acr_compute_node_list_get_size(node_list); ++i) {
    fprintf(out, "|---| %zu\n", i);
    pprint_acr_compute_node(out,
        acr_compute_node_list_get_node(i, node_list),
        indent_level + 1);
  }

}

void pprint_acr_option_list(FILE *out,
                            acr_option_list opt_list,
                            size_t list_size,
                            size_t indent_level) {
  for (size_t i = 0; i < list_size; ++i) {
    const acr_option opt = acr_option_list_get_option(i, opt_list);
    pprint_acr_option(out, opt, indent_level + 1);
  }
  pprint_acr_indent(out, indent_level);
  fprintf(out, "|\n");
}
