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

#include "acr/pragma_struct.h"

#include <string.h>

acr_option acr_new_alternative_function(const char* alternative_name,
                                        const char* function_to_swap,
                                        const char* replacement_function) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_function;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  option->options.alternative.name_of_object_to_swap =
    acr_strdup(function_to_swap);
  option->options.alternative.swapped_by.replacement_function =
    acr_strdup(replacement_function);
  return option;
}


acr_option acr_new_alternative_parameter(const char* alternative_name,
                                         const char* parameter_to_swap,
                                         long int replacement_value){
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_parameter;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  option->options.alternative.name_of_object_to_swap =
    acr_strdup(parameter_to_swap);
  option->options.alternative.swapped_by.replacement_value = replacement_value;
  return option;
}

acr_option acr_new_destroy(size_t pragma_row_position){
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_destroy;
  option->options.destroy.pragma_row_position = pragma_row_position;
  return option;
}

acr_option acr_new_grid(unsigned long int grid_size){
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_grid;
  option->options.grid.grid_size = grid_size;
  return option;
}

acr_option acr_new_init(const char* function_name,
                        size_t pragma_row_position,
                        unsigned long int num_parameters,
                        acr_parameter_declaration* parameters_list) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_init;
  option->options.init.pragma_row_position = pragma_row_position;
  option->options.init.num_parameters = num_parameters;
  option->options.init.parameters_list = parameters_list;
  option->options.init.function_name = acr_strdup(function_name);
  return option;
}

acr_parameter_declaration* acr_new_parameter_declaration_list(
    unsigned long int list_size) {
  acr_parameter_declaration* list = malloc(list_size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}


acr_parameter_specifier* acr_new_parameter_specifier_list(
    unsigned long int list_size) {
  acr_parameter_specifier* list = malloc(list_size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}

acr_option acr_new_monitor(
    const struct acr_array_declaration* array_declaration,
    enum acr_monitor_processing_funtion processing_function,
    const char* filter_name) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_monitor;
  option->options.monitor.data_monitored = *array_declaration;
  option->options.monitor.processing_function = processing_function;
  option->options.monitor.filter_name = acr_strdup(filter_name);
  return option;
}

acr_option acr_new_strategy_direct_int(const char* strategy_name,
                                       long int matching_value) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  option->options.strategy.strategy_type = acr_strategy_direct;
  option->options.strategy.value_type = acr_strategy_integer;
  option->options.strategy.boundaries[0].value_int = matching_value;
  option->options.strategy.boundaries[1].value_int = 0l;
  return option;
}

acr_option acr_new_strategy_range_int(const char* strategy_name,
                                      long int matching_value[2]) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  if (matching_value[0] == matching_value[1])
    option->options.strategy.strategy_type = acr_strategy_direct;
  else
    option->options.strategy.strategy_type = acr_strategy_range;
  option->options.strategy.value_type = acr_strategy_integer;
  option->options.strategy.boundaries[0].value_int = matching_value[0];
  option->options.strategy.boundaries[1].value_int = matching_value[1];
  return option;
}

acr_option acr_new_strategy_direct_float(const char* strategy_name,
                                         float matching_value) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  option->options.strategy.strategy_type = acr_strategy_direct;
  option->options.strategy.value_type = acr_strategy_floating_point;
  option->options.strategy.boundaries[0].value_float = matching_value;
  option->options.strategy.boundaries[1].value_float = 0.f;
  return option;
}

acr_option acr_new_strategy_range_float(const char* strategy_name,
                                        float matching_value[2]) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  if (matching_value[0] == matching_value[1])
    option->options.strategy.strategy_type = acr_strategy_direct;
  else
    option->options.strategy.strategy_type = acr_strategy_range;
  option->options.strategy.value_type = acr_strategy_floating_point;
  option->options.strategy.boundaries[0].value_float = matching_value[0];
  option->options.strategy.boundaries[1].value_float = matching_value[1];
  return option;
}

acr_option_list acr_new_option_list(unsigned long int size) {
  acr_option_list list = malloc(size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}

static void acr_free_alternative(acr_alternative* alternative) {
  if (!alternative)
    return;
  if (alternative->type == acr_alternative_function)
    free(alternative->swapped_by.replacement_function);
  free(alternative->alternative_name);
  free(alternative->name_of_object_to_swap);
}

static void acr_free_parameter_specifier_list(unsigned long int num_specifiers,
    acr_parameter_specifier* parameter_specifiers_list) {
  if (!parameter_specifiers_list)
    return;
  for(unsigned long int i = 0; i < num_specifiers; ++i) {
    free(parameter_specifiers_list[i].specifier);
  }
  free(parameter_specifiers_list);
}

static void acr_free_parameter_declaration_list(unsigned long int num_parameters,
    acr_parameter_declaration* parameter_list) {
  if (!parameter_list)
    return;
  for(unsigned long int i = 0; i < num_parameters; ++i) {
    free(parameter_list[i].parameter_name);
    acr_free_parameter_specifier_list(parameter_list[i].num_specifiers,
        parameter_list[i].parameter_specifiers_list);
  }
  free(parameter_list);
}

static void acr_free_init(acr_init* init) {
  if (!init)
    return;
  free(init->function_name);
  acr_free_parameter_declaration_list(init->num_parameters,
      init->parameters_list);
}

void acr_free_acr_array_declaration(
    struct acr_array_declaration* array_declaration) {
  if (!array_declaration)
    return;
  acr_free_parameter_specifier_list(array_declaration->num_specifiers,
    array_declaration->parameter_specifiers_list);
  free(array_declaration->array_name);
  acr_free_array_dimensions_list(array_declaration->num_dimensions,
      array_declaration->array_dimensions_list);
}

static void acr_free_monitor(acr_monitor* monitor) {
  if (!monitor)
    return;
  free(monitor->filter_name);
  acr_free_acr_array_declaration(&monitor->data_monitored);
}

static void acr_free_strategy(acr_strategy* strategy) {
  if (!strategy)
    return;
  free(strategy->strategy_name);
}

void acr_free_option(acr_option opt) {
  if (!opt)
    return;
  switch (opt->type) {
    case acr_type_alternative:
      acr_free_alternative(&opt->options.alternative);
      break;
    case acr_type_destroy:
    case acr_type_grid:
      break;
    case acr_type_init:
      acr_free_init(&opt->options.init);
      break;
    case acr_type_monitor:
      acr_free_monitor(&opt->options.monitor);
      break;
    case acr_type_strategy:
      acr_free_strategy(&opt->options.strategy);
      break;
    case acr_type_unknown:
      break;
  }

  free(opt);
}

void acr_free_option_list(acr_option_list option_list, unsigned long int size) {
  if (!option_list)
    return;
  for (unsigned long int i = 0; i < size; ++i) {
    acr_free_option(acr_option_list_get_option(i, option_list));
  }
  free(option_list);
}

acr_compute_node acr_new_compute_node(unsigned long int list_size,
    acr_option_list list) {
  acr_compute_node node = malloc(sizeof(*node));
  acr_try_or_die(node == NULL, "Malloc");
  node->option_list = list;
  node->list_size = list_size;
  return node;
}

void acr_free_compute_node(acr_compute_node node) {
  if (!node)
    return;
  acr_free_option_list(node->option_list, node->list_size);
  free(node);
}

acr_array_dimensions_list acr_new_array_dimensions_list(unsigned long int size) {
  acr_array_dimensions_list dim = malloc(size * sizeof(*dim));
  acr_try_or_die(dim == NULL, "Malloc");
  return dim;
}

acr_compute_node_list acr_new_compute_node_list(unsigned long list_size) {
  acr_compute_node_list list = malloc(sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  list->list_size = list_size;
  list->compute_node_list = malloc(list_size * sizeof(*list->compute_node_list));
  acr_try_or_die(list->compute_node_list == NULL, "Malloc");
  return list;
}

static void acr_compute_node_delete_option_from_position(
    unsigned long position,
    acr_compute_node node) {
  acr_option_list option_list = acr_compute_node_get_option_list(node);
  unsigned long option_list_size = acr_compute_node_get_option_list_size(node);

  if (position < option_list_size) {
    acr_free_option(option_list[position]);
    for (unsigned long i = position; i + 1ul < option_list_size; ++i) {
      option_list[i] = option_list[i + 1ul];
    }
    option_list_size -= 1ul;
    node->option_list = realloc(node->option_list, option_list_size *
                                                   sizeof(*node->option_list));
    node->list_size = option_list_size;
  }
}

static bool acr_option_are_same_alternative(const acr_option alternative1,
                                            const acr_option alternative2) {
  if (acr_option_get_type(alternative1) == acr_type_alternative &&
      acr_option_get_type(alternative2) == acr_type_alternative) {
    char* name1 = alternative1->options.alternative.alternative_name;
    char* name2 = alternative2->options.alternative.alternative_name;
    return strcmp(name1, name2) == 0;
  }
  return false;
}

static bool acr_strategy_first_included_in_second(const acr_option strategy1,
                                                  const acr_option strategy2) {
  if (acr_option_get_type(strategy1) == acr_type_strategy &&
      acr_option_get_type(strategy2) == acr_type_strategy) {
    long val_int1[2];
    float val_float1[2];
    long val_int2[2];
    float val_float2[2];
    switch (acr_strategy_get_strategy_type(strategy1)) {
      case acr_strategy_direct:
        switch (acr_strategy_get_strategy_type(strategy2)) {
          case acr_strategy_direct:
            if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer &&
                acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
              acr_strategy_get_int_val(strategy1, val_int1);
              acr_strategy_get_int_val(strategy2, val_int2);
              return val_int1[0] == val_int2[0];
            } else {
              if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_float_val(strategy2, val_float2);
                return ((float) val_int1[0]) == val_float2[0];
              } else {
                if (acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_int_val(strategy2, val_int2);
                  return val_float1[0] == ((float) val_int2[0]);
                }
                else {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_float_val(strategy2, val_float2);
                  return val_float1[0] == val_int2[0];
                }
              }
            }
            break;
          case acr_strategy_range:
            if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer &&
                acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_int_val(strategy2, val_int2);
                return val_int1[0] >= val_int2[0] && val_int1[0] <= val_int2[1];
            } else {
              if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_float_val(strategy2, val_float2);
                return ((float) val_int1[0]) >= val_float2[0] &&
                       ((float) val_int1[0]) <= val_float2[1];
              } else {
                if (acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_int_val(strategy2, val_int2);
                  return val_float1[0] >= ((float) val_int2[0]) &&
                         val_float1[0] <= ((float) val_int2[1]);
                }
                else {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_float_val(strategy2, val_float2);
                  return val_float1[0] >= val_float2[0] &&
                         val_float1[0] <= val_float2[1];
                }
              }
            }
            break;
          case acr_strategy_unknown:
            break;
        }
        break;
      case acr_strategy_range:
        switch (acr_strategy_get_strategy_type(strategy2)) {
          case acr_strategy_direct:
            return false;
            break;
          case acr_strategy_range:
            if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer &&
                acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
              acr_strategy_get_int_val(strategy1, val_int1);
              acr_strategy_get_int_val(strategy2, val_int2);
              return val_int1[0] >= val_int2[0] && val_int1[1] <= val_int2[1];
            } else {
              if (acr_strategy_get_value_type(strategy1) ==
                  acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_float_val(strategy2, val_float2);
                return ((float) val_int1[0]) >= val_float2[0] &&
                       ((float) val_int1[1]) <= val_float2[1];
              } else {
                if (acr_strategy_get_value_type(strategy2) ==
                    acr_strategy_integer) {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_int_val(strategy2, val_int2);
                  return val_float1[0] >= ((float) val_int2[0]) &&
                        val_float1[1] <= ((float) val_int2[1]);
                } else {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_float_val(strategy2, val_float2);
                  return val_float1[0] >= val_float2[0] &&
                        val_float1[1] <= val_float2[1];
                }
              }
            }
            break;
          case acr_strategy_unknown:
            break;
        }
        break;
      case acr_strategy_unknown:
        break;
    }
  }
  return false;
}

#include "acr/print.h"

void acr_simlpify_compute_node(acr_compute_node node) {
  unsigned long last_init = 0;
  acr_option_list option_list;
  for (unsigned long i = 0; i < acr_compute_node_get_option_list_size(node); ++i) {
    option_list = acr_compute_node_get_option_list(node);
    switch (acr_option_get_type(acr_option_list_get_option(i, option_list))) {
      case acr_type_init:
        last_init = i;
        break;
      case acr_type_alternative:
        for (unsigned int j = last_init + 1; j < i; ++j) {
          option_list = acr_compute_node_get_option_list(node);
          acr_option to_compare = acr_option_list_get_option(j, option_list);
          if (acr_option_are_same_alternative(
                acr_option_list_get_option(i, option_list), to_compare)) {
            acr_compute_node_delete_option_from_position(j, node);
            i -= 1ul;
            j -= 1ul;
          }
        }
        break;
      case acr_type_grid:
      case acr_type_monitor:
      case acr_type_destroy:
        for (unsigned int j = last_init + 1; j < i; ++j) {
          option_list = acr_compute_node_get_option_list(node);
          acr_option to_compare = acr_option_list_get_option(j, option_list);
          if (acr_option_get_type(to_compare) ==
              acr_option_get_type(acr_option_list_get_option(i, option_list))) {
            acr_compute_node_delete_option_from_position(j, node);
            i -= 1ul;
            j -= 1ul;
          }
        }
        break;
      case acr_type_strategy:
        for (unsigned int j = last_init + 1; j < i; ++j) {
            option_list = acr_compute_node_get_option_list(node);
            acr_option to_compare = acr_option_list_get_option(j, option_list);
          if (acr_strategy_first_included_in_second(to_compare, acr_option_list_get_option(i, option_list))) {
            acr_compute_node_delete_option_from_position(j, node);
            i -= 1ul;
            j -= 1ul;
          } else {
            if (acr_strategy_first_included_in_second(
                  acr_option_list_get_option(i, option_list), to_compare)) {
              acr_compute_node_delete_option_from_position(i, node);
              i -= 1ul;
              break;
            }
          }
        }
        break;
      case acr_type_unknown:
        break;
    }
  }
}
