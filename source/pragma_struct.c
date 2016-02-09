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
#include "acr/utils.h"

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
                                         int replacement_value){
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_function;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  option->options.alternative.name_of_object_to_swap =
    acr_strdup(parameter_to_swap);
  option->options.alternative.swapped_by.replacement_value = replacement_value;
  return option;
}

acr_option acr_new_destroy(unsigned int pragma_row_position){
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_destroy;
  option->options.destroy.pragma_row_position = pragma_row_position;
  return option;
}

acr_option acr_new_grid(unsigned int grid_size){
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_grid;
  option->options.grid.grid_size = grid_size;
  return option;
}

acr_option acr_new_init(const char* function_name,
                        unsigned int pragma_row_position,
                        unsigned int num_parameters,
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
    unsigned int list_size) {
  acr_parameter_declaration* list = malloc(list_size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}

void acr_set_parameter_declaration(
    const char* parameter_name,
    unsigned int num_specifiers,
    acr_parameter_specifier* specifier_list,
    acr_parameter_declaration* declaration_to_set) {
  declaration_to_set->parameter_specifiers_list = specifier_list;
  declaration_to_set->num_specifiers = num_specifiers;
  declaration_to_set->parameter_name = acr_strdup(parameter_name);
}

acr_parameter_specifier* acr_new_parameter_specifier_list(
    unsigned int list_size) {
  acr_parameter_specifier* list = malloc(list_size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}

void acr_set_parameter_specifier(const char* specifier_name,
                                 unsigned int pointer_depth,
                                 acr_parameter_specifier* specifier) {
  specifier->specifier = acr_strdup(specifier_name);
  specifier->pointer_depth = pointer_depth;
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

void acr_set_array_declaration(unsigned int num_specifiers,
                               acr_parameter_specifier* parameters_list,
                               const char* array_name,
                               unsigned int num_dimensions,
                               unsigned int* array_dimensions,
                               struct acr_array_declaration* array_declaration) {
  array_declaration->array_dimensions_list = array_dimensions;
  array_declaration->num_dimensions = num_dimensions;
  array_declaration->num_specifiers = num_specifiers;
  array_declaration->array_name = acr_strdup(array_name);
  array_declaration->parameter_specifiers_list = parameters_list;
}

acr_option acr_new_strategy_direct_int(const char* strategy_name,
                                       int matching_value) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  option->options.strategy.strategy_type = acr_strategy_direct;
  option->options.strategy.value_type = acr_strategy_integer;
  option->options.strategy.boundaries[0].value_int = matching_value;
  return option;
}

acr_option acr_new_strategy_range_int(const char* strategy_name,
                                      int matching_value[2]) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
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
  return option;
}

acr_option acr_new_strategy_range_float(const char* strategy_name,
                                        float matching_value[2]) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  option->options.strategy.strategy_type = acr_strategy_range;
  option->options.strategy.value_type = acr_strategy_floating_point;
  option->options.strategy.boundaries[0].value_float = matching_value[0];
  option->options.strategy.boundaries[1].value_float = matching_value[1];
  return option;
}

acr_option_list acr_new_option_list(unsigned int size) {
  acr_option_list list = malloc(size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}

static void acr_free_alternative(acr_alternative* alternative) {
  if (alternative->type == acr_alternative_function)
    free(alternative->swapped_by.replacement_function);
  free(alternative->alternative_name);
  free(alternative->name_of_object_to_swap);
}

static void acr_free_parameter_specifier_list(unsigned int num_specifiers,
    acr_parameter_specifier* parameter_specifiers_list) {
  for(unsigned int i = 0; i < num_specifiers; ++i) {
    free(parameter_specifiers_list[i].specifier);
  }
  free(parameter_specifiers_list);
}

static void acr_free_parameter_declaration_list(unsigned int num_parameters,
    acr_parameter_declaration* parameter_list) {
  for(unsigned int i = 0; i < num_parameters; ++i) {
    free(parameter_list[i].parameter_name);
    acr_free_parameter_specifier_list(parameter_list[i].num_specifiers,
        parameter_list[i].parameter_specifiers_list);
  }
  free(parameter_list);
}

static void acr_free_init(acr_init* init) {
  free(init->function_name);
  acr_free_parameter_declaration_list(init->num_parameters,
      init->parameters_list);
}

static void acr_free_acr_array_declaration(
    struct acr_array_declaration* array_declaration) {
  acr_free_parameter_specifier_list(array_declaration->num_specifiers,
    array_declaration->parameter_specifiers_list);
  free(array_declaration->array_name);
  free(array_declaration->array_dimensions_list);
}

static void acr_free_monitor(acr_monitor* monitor) {
  free(monitor->filter_name);
  acr_free_acr_array_declaration(&monitor->data_monitored);
}

static void acr_free_strategy(acr_strategy* strategy) {
  free(strategy->strategy_name);
}

void acr_free_option(acr_option* option) {
  acr_option opt = *option;
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
  option = NULL;
}
