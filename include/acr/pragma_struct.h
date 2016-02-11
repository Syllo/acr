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
#include "acr/utils.h"

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
    long int replacement_value;
    char* replacement_function;
  } swapped_by;
} acr_alternative;

typedef struct acr_destroy {
  size_t pragma_row_position;
} acr_destroy;

typedef struct acr_grid {
  unsigned long int grid_size;
} acr_grid;

typedef struct acr_parameter_specifier {
  unsigned long int pointer_depth;
  char* specifier;
} acr_parameter_specifier, *acr_parameter_specifier_list;

typedef struct acr_parameter_declaration {
  unsigned long int num_specifiers;
  acr_parameter_specifier_list parameter_specifiers_list;
  char* parameter_name;
} acr_parameter_declaration, *acr_parameter_declaration_list;

typedef struct acr_init {
  size_t pragma_row_position;
  char* function_name;
  unsigned long int num_parameters;
  acr_parameter_declaration_list parameters_list;
} acr_init;

enum acr_array_dimensions_type{
  acr_array_dimension_uinteger,
  acr_array_dimension_parameter
};

typedef struct acr_array_dimensions{
  enum acr_array_dimensions_type type;
  union {
    unsigned long int dimension;
    char* parameter_name;
  } value;
} acr_array_dimensions, *acr_array_dimensions_list;

typedef struct acr_array_declaration {
  unsigned long int num_specifiers;
  acr_parameter_specifier_list parameter_specifiers_list;
  char* array_name;
  unsigned long int num_dimensions;
  acr_array_dimensions_list array_dimensions_list;
} acr_array_declaration;

enum acr_monitor_processing_funtion {
  acr_monitor_function_min = 0,
  acr_monitor_function_max,
  acr_monitor_function_unknown,
};

typedef struct acr_monitor {
  acr_array_declaration data_monitored;
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
    long int value_int;
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
} *acr_option, **acr_option_list;

typedef struct acr_compute_node {
  unsigned long int list_size;
  acr_option_list option_list;
} *acr_compute_node;

typedef struct acr_compute_node_list {
  unsigned long int list_size;
  acr_compute_node* compute_node_list;
} *acr_compute_node_list;

static inline enum acr_type acr_get_type(acr_option option) {
  return option->type;
}

acr_option acr_new_alternative_function(const char* alternative_name,
                                        const char* function_to_swap,
                                        const char* replacement_function);

acr_option acr_new_alternative_parameter(const char* alternative_name,
                                         const char* parameter_to_swap,
                                         long int replacement_value);

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

static inline long int acr_alternative_get_replacement_parameter(
    const acr_option option){
  return acr_alternative_get_type(option) == acr_alternative_parameter ?
    option->options.alternative.swapped_by.replacement_value :
    0;
}

acr_option acr_new_destroy(size_t pragma_row_position);

static inline size_t acr_destroy_get_row_position(
    const acr_option option) {
  return option->options.destroy.pragma_row_position;
}

acr_option acr_new_grid(unsigned long int grid_size);

static inline unsigned long int acr_grid_get_grid_size(
    const acr_option option) {
  return option->options.grid.grid_size;
}

acr_option acr_new_init(const char* function_name,
                        size_t pragma_row_position,
                        unsigned long int num_parameters,
                        acr_parameter_declaration_list parameters_list);

static inline char* acr_init_get_function_name(const acr_option option) {
  return option->options.init.function_name;
}

static inline size_t acr_init_get_pragma_row_position(
    const acr_option option) {
  return option->options.init.pragma_row_position;
}

static inline unsigned long int acr_init_get_num_parameters(
    const acr_option option) {
  return option->options.init.num_parameters;
}

static inline acr_parameter_declaration_list acr_init_get_parameter_list(
    const acr_option option) {
  return option->options.init.parameters_list;
}


acr_parameter_declaration_list acr_new_parameter_declaration_list(
    unsigned long int list_size);

static inline void acr_set_parameter_declaration(
    const char* parameter_name,
    unsigned long int num_specifiers,
    acr_parameter_specifier* specifier_list,
    acr_parameter_declaration* declaration_to_set) {
  declaration_to_set->parameter_specifiers_list = specifier_list;
  declaration_to_set->num_specifiers = num_specifiers;
  declaration_to_set->parameter_name = acr_strdup(parameter_name);
}

static inline char* acr_parameter_declaration_get_parameter_name(
    const acr_parameter_declaration_list declaration_list,
    unsigned long int position_in_list) {
  return declaration_list[position_in_list].parameter_name;
}

static inline unsigned long int acr_parameter_declaration_get_num_specifiers(
    const acr_parameter_declaration_list declaration_list,
    unsigned long int position_in_list) {
  return declaration_list[position_in_list].num_specifiers;
}

static inline acr_parameter_specifier_list acr_parameter_declaration_get_specif_list(
    const acr_parameter_declaration_list declaration_list,
    unsigned long int position_in_list) {
  return declaration_list[position_in_list].parameter_specifiers_list;
}

acr_parameter_specifier_list acr_new_parameter_specifier_list(
    unsigned long int list_size);

static inline void acr_set_parameter_specifier(
    const char* specifier_name,
    unsigned long int pointer_depth,
    acr_parameter_specifier* specifier) {
  specifier->specifier = acr_strdup(specifier_name);
  specifier->pointer_depth = pointer_depth;
}

static inline bool acr_parameter_specifier_is_pointer(
    const acr_parameter_specifier_list specifier,
    unsigned long int position_in_list) {
  return specifier[position_in_list].pointer_depth > 0;
}

static inline unsigned long int acr_parameter_specifier_get_pointer_depth(
    const acr_parameter_specifier_list specifier,
    unsigned long int position_in_list) {
  return specifier[position_in_list].pointer_depth;
}

static inline char* acr_parameter_specifier_get_specifier(
    const acr_parameter_specifier_list specifier,
    unsigned long int position_in_list) {
  return specifier[position_in_list].specifier;
}

acr_option acr_new_monitor(
    const acr_array_declaration* array_declaration,
    enum acr_monitor_processing_funtion processing_function,
    const char* filter_name);

static inline acr_array_declaration* acr_monitor_get_array_declaration(
    acr_option option) {
  return &option->options.monitor.data_monitored;
}

static inline enum acr_monitor_processing_funtion acr_monitor_get_function(
    acr_option option) {
  return option->options.monitor.processing_function;
}

static inline char* acr_monitor_get_filter_name(acr_option option) {
  return option->options.monitor.filter_name;
}

static inline void acr_set_array_declaration(
    unsigned long int num_specifiers,
    acr_parameter_specifier* parameters_list,
    const char* array_name,
    unsigned long int num_dimensions,
    acr_array_dimensions_list array_dimensions,
    struct acr_array_declaration* array_declaration) {
  array_declaration->array_dimensions_list = array_dimensions;
  array_declaration->num_dimensions = num_dimensions;
  array_declaration->num_specifiers = num_specifiers;
  array_declaration->array_name = acr_strdup(array_name);
  array_declaration->parameter_specifiers_list = parameters_list;
}

static inline unsigned long int acr_array_decl_get_num_specifiers(
    acr_array_declaration* declaration) {
  return declaration->num_specifiers;
}

static inline acr_parameter_specifier_list acr_array_decl_get_specifiers_list(
    acr_array_declaration* declaration) {
  return declaration->parameter_specifiers_list;
}

static inline char* acr_array_decl_get_array_name(
    acr_array_declaration* declaration) {
  return declaration->array_name;
}

static inline unsigned long int acr_array_decl_get_num_dimensions(
    acr_array_declaration* declaration) {
  return declaration->num_dimensions;
}

static inline acr_array_dimensions_list acr_array_decl_get_dimensions_list(
    acr_array_declaration* declaration) {
  return declaration->array_dimensions_list;
}

acr_option acr_new_strategy_direct_int(const char* strategy_name,
                                       long int matching_value);

acr_option acr_new_strategy_direct_float(const char* strategy_name,
                                         float matching_value);

acr_option acr_new_strategy_range_int(const char* strategy_name,
                                      long int matching_value[2]);

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
    long int values[2]){
  values[0] = option->options.strategy.boundaries[0].value_int;
  if(option->options.strategy.strategy_type == acr_strategy_range) {
    values[1] = option->options.strategy.boundaries[1].value_int;
  }
}

acr_option_list acr_new_option_list(unsigned long int size);

static inline void acr_option_list_set_option(acr_option option,
                                              unsigned long int position,
                                              acr_option_list list) {
  list[position] = option;
}

static inline acr_option acr_option_list_get_option(unsigned long int position,
                                                    acr_option_list list) {
  return list[position];
}

void acr_free_option(acr_option option);

void acr_free_option_list(acr_option_list option_list, unsigned long int size);

acr_compute_node acr_new_compute_node(unsigned long int list_size,
    acr_option_list list);

void acr_free_compute_node(acr_compute_node node);

static inline acr_option_list acr_compute_node_get_option_list(
    acr_compute_node node) {
  return node->option_list;
}

static inline unsigned long int acr_compute_node_get_option_list_size(
    acr_compute_node node) {
  return node->list_size;
}

void acr_free_acr_array_declaration(
    struct acr_array_declaration* array_declaration);

acr_array_dimensions_list acr_new_array_dimensions_list(unsigned long int size);

static inline void acr_free_array_dimensions_list(
    unsigned long list_size,
    acr_array_dimensions_list list) {
  for (unsigned long int i = 0; i < list_size; ++i) {
    if (list[i].type == acr_array_dimension_parameter) {
      free(list[i].value.parameter_name);
    }
  }
  free(list);
}

static inline enum acr_array_dimensions_type acr_array_dimensions_get_type(
    unsigned int position,
    const acr_array_dimensions_list dimension_list) {
  return dimension_list[position].type;
}

static inline unsigned long int acr_array_dimensions_get_dim_size(
    unsigned long int position,
    const acr_array_dimensions_list dimension_list) {
  return dimension_list[position].value.dimension;
}

static inline char* acr_array_dimensions_get_dim_name(
    unsigned long int position,
    const acr_array_dimensions_list dimension_list) {
  return dimension_list[position].value.parameter_name;
}

static inline void acr_array_dimensions_set_dim_name(
    unsigned long int position,
    const char* name,
    acr_array_dimensions_list dimension_list) {
  dimension_list[position].type = acr_array_dimension_parameter;
  dimension_list[position].value.parameter_name = acr_strdup(name);
}

static inline void acr_array_dimensions_set_dim_size(
    unsigned long int position,
    unsigned long int dim_size,
    acr_array_dimensions_list dimension_list) {
  dimension_list[position].type = acr_array_dimension_uinteger;
  dimension_list[position].value.dimension = dim_size;
}

acr_compute_node_list acr_new_compute_node_list(unsigned long list_size);

void acr_free_compute_node_list(acr_compute_node_list list);

static inline acr_compute_node acr_compute_node_list_set_node(
    unsigned long position,
    const acr_compute_node node,
    acr_compute_node_list node_list) {
  return node_list->compute_node_list[position] = node;
}

static inline acr_compute_node acr_compute_node_list_get_node(
    unsigned long position,
    const acr_compute_node_list node_list) {
  return node_list->compute_node_list[position];
}

static inline unsigned long acr_compute_node_list_get_size(
    const acr_compute_node_list node_list) {
  return node_list->list_size;
}

#endif // __ACR_PRAGMA_STRUCT_H
