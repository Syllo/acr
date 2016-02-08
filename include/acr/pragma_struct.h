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

#ifndef __ACR_PRAGMA_STRUCT_H
#define __ACR_PRAGMA_STRUCT_H

#include <stdbool.h>
#include <stdlib.h>

enum acr_alternative_type {
  acr_alternative_parameter = 0,
  acr_alternative_function,
  acr_alternative_unknown,
};

typedef struct acr_alternative {
  enum acr_alternative_type type;
  char* alternative_name;
  char* name_of_object_to_swap;
  union {
    int replacement_value;
    char* replacement_function;
  } swapped_by;
} acr_alternative;

typedef struct acr_destroy {
  unsigned int pragma_row_position;
} acr_destroy;

typedef struct acr_grid {
  unsigned int grid_size;
} acr_grid;

typedef struct acr_parameter_specifier {
  unsigned int pointer_depth;
  char* specifier;
} acr_parameter_specifier;

typedef struct acr_parameter_declaration {
  unsigned int num_specifiers;
  acr_parameter_specifier* parameter_specifiers_list;
  char* parameter_name;
} acr_parameter_declaration;

typedef struct acr_init {
  unsigned int pragma_row_position;
  char* function_name;
  unsigned int num_parameters;
  acr_parameter_declaration* parameters_list;
} acr_init;

struct acr_array_declaration {
  unsigned int num_specifiers;
  acr_parameter_specifier* parameter_specifiers_list;
  char* array_name;
  unsigned int num_dimensions;
  unsigned int* array_dimensions_list;
};

enum acr_monitor_processing_funtion {
  acr_monitor_function_min = 0,
  acr_monitor_function_max,
  acr_monitor_function_unknown,
};

typedef struct acr_monitor {
  struct acr_array_declaration data_monitored;
  enum acr_monitor_processing_funtion processing_function;
  char* filter_name;
} acr_monitor;

enum acr_strategy_type {
  acr_strategy_direct = 0,
  acr_strategy_range,
  acr_strategy_unknown,
};

enum acr_strategy_value_type {
  acr_strategy_integer = 0,
  acr_strategy_floating_point,
};

typedef struct acr_strategy {
  enum acr_strategy_type strategy_type;
  enum acr_strategy_value_type value_type;
  union {
    int value_int;
    float value_float;
  } boundaries[2];
  char* strategy_name;
} acr_strategy;

enum acr_type {
  acr_type_alternative = 0,
  acr_type_destroy,
  acr_type_grid,
  acr_type_init,
  acr_type_monitor,
  acr_type_strategy,
  acr_type_unknown,
};

typedef struct acr_option {
  enum acr_type type;
  union {
    acr_alternative alternative;
    acr_destroy destroy;
    acr_grid grid;
    acr_init init;
    acr_monitor monitor;
    acr_strategy strategy;
  } options;
} *acr_option;

typedef acr_option* acr_option_list;

typedef struct acr_compute_node {
  unsigned int list_size;
  acr_option_list option_list;
} acr_compute_node;

static inline enum acr_type acr_get_type(acr_option option) {
  return option->type;
}

acr_option acr_new_alternative_function(const char* alternative_name,
                                          const char* function_to_swap,
                                          const char* replacement_function);

acr_option acr_new_alternative_parameter(const char* alternative_name,
                                         const char* parameter_to_swap,
                                         int replacement_value);

static inline enum acr_alternative_type acr_alternative_get_type(
    const acr_option option) {
  return option->options.alternative.type;
}

static inline char* acr_alternative_get_alternative_name(
    const acr_option option) {
  return option->options.alternative.alternative_name;
}


static inline char* acr_alternative_get_object_to_swap_name(
    const acr_option option){
  return option->options.alternative.name_of_object_to_swap;
}

static inline char* acr_alternative_get_replacement_function(
    const acr_option option){
  return acr_alternative_get_type(option) == acr_alternative_function ?
    option->options.alternative.swapped_by.replacement_function :
    NULL;
}

static inline int acr_alternative_get_replacement_parameter(
    const acr_option option){
  return acr_alternative_get_type(option) == acr_alternative_parameter ?
    option->options.alternative.swapped_by.replacement_value :
    0;
}

acr_option acr_new_destroy(unsigned int pragma_row_position);

static inline unsigned int acr_destroy_get_row_position(
    const acr_option option) {
  return option->options.destroy.pragma_row_position;
}

acr_option acr_new_grid(unsigned int grid_size);

static inline unsigned int acr_grid_get_grid_size(
    const acr_option option) {
  return option->options.grid.grid_size;
}

acr_option acr_new_init(const char* function_name,
                        unsigned int pragma_row_position,
                        unsigned int num_parameters,
                        acr_parameter_declaration* parameters_list);

static inline char* acr_init_get_function_name(const acr_option option) {
  return option->options.init.function_name;
}

static inline unsigned int acr_init_get_pragma_row_position(
    const acr_option option) {
  return option->options.init.pragma_row_position;
}

static inline unsigned int acr_init_get_num_parameters(
    const acr_option option) {
  return option->options.init.num_parameters;
}

static inline acr_parameter_declaration* acr_init_get_parameter_list(
    const acr_option option) {
  return option->options.init.parameters_list;
}


acr_parameter_declaration* acr_new_parameter_declaration_list(
    unsigned int list_size);

void acr_set_parameter_declaration(
    const char* parameter_name,
    unsigned int num_specifiers,
    acr_parameter_specifier* specifier_list,
    acr_parameter_declaration* declaration_to_set);

static inline char* acr_parameter_declaration_get_parameter_name(
    const acr_parameter_declaration* declaration_list,
    unsigned int position_in_list) {
  return declaration_list[position_in_list].parameter_name;
}

static inline unsigned int acr_parameter_declaration_get_num_specifiers(
    const acr_parameter_declaration* declaration_list,
    unsigned int position_in_list) {
  return declaration_list[position_in_list].num_specifiers;
}

static inline acr_parameter_specifier* acr_parameter_declaration_get_specif_list(
    const acr_parameter_declaration* declaration_list,
    unsigned int position_in_list) {
  return declaration_list[position_in_list].parameter_specifiers_list;
}

acr_parameter_specifier* acr_new_parameter_specifier_list(
    unsigned int list_size);

void acr_set_parameter_specifier(const char* specifier_name,
                                 unsigned int pointer_depth,
                                 acr_parameter_specifier* specifier);

static inline bool acr_parameter_specifier_is_pointer(
    const acr_parameter_specifier* specifier,
    unsigned int position_in_list) {
  return specifier[position_in_list].pointer_depth > 0;
}

static inline unsigned int acr_parameter_specifier_get_pointer_depth(
    const acr_parameter_specifier* specifier,
    unsigned int position_in_list) {
  return specifier[position_in_list].pointer_depth;
}

static inline char* acr_parameter_specifier_get_specifier(
    const acr_parameter_specifier* specifier,
    unsigned int position_in_list) {
  return specifier[position_in_list].specifier;
}

acr_option acr_new_monitor(
    const struct acr_array_declaration* array_declaration,
    enum acr_monitor_processing_funtion processing_function,
    const char* filter_name);

void acr_set_array_declaration(unsigned int num_specifiers,
                               acr_parameter_specifier* parameters_list,
                               const char* array_name,
                               unsigned int num_dimensions,
                               unsigned int* array_dimensions,
                               struct acr_array_declaration* array_declaration);

acr_option acr_new_strategy_direct_int(const char* strategy_name,
                                       int matching_value);

acr_option acr_new_strategy_direct_float(const char* strategy_name,
                                         float matching_value);

acr_option acr_new_strategy_range_int(const char* strategy_name,
                                      int matching_value[2]);

acr_option acr_new_strategy_range_float(const char* strategy_name,
                                        float matching_value[2]);

static inline enum acr_strategy_type acr_strategy_get_strategy_type(
    const acr_option option) {
  return option->options.strategy.strategy_type;
}

static inline char* acr_strategy_get_name(const acr_option option) {
  return option->options.strategy.strategy_name;
}

static inline enum acr_strategy_value_type acr_strategy_get_value_type(
    const acr_option option) {
  return option->options.strategy.value_type;
}

static inline void acr_strategy_populate_float_val(
    const acr_option option,
    float values[2]){
  values[0] = option->options.strategy.boundaries[0].value_float;
  if(option->options.strategy.strategy_type == acr_strategy_range) {
    values[1] = option->options.strategy.boundaries[1].value_float;
  }
}

static inline void acr_strategy_populate_int_val(
    const acr_option option,
    int values[2]){
  values[0] = option->options.strategy.boundaries[0].value_int;
  if(option->options.strategy.strategy_type == acr_strategy_range) {
    values[1] = option->options.strategy.boundaries[1].value_int;
  }
}

acr_option_list acr_new_option_list(unsigned int size);

void acr_free_option(acr_option* option);

void acr_free_option_list(acr_option_list* option_list);

#endif // __ACR_PRAGMA_STRUCT_H
