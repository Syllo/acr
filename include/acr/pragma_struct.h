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

#ifndef __PRAGMA_STRUCT_H
#define __PRAGMA_STRUCT_H

#include <stdbool.h>

enum acr_alternative_type {
  acr_alternative_parameter,
  acr_alternative_function,
};

typedef struct acr_alternative {
  enum acr_alternative_type type;
  char* alternative_name;
  char* name_of_object_to_swap;
  union swapped_by {
    int replacement_value;
    char* replacement_function;
  };
} acr_alternative;

typedef struct acr_destroy {
  unsigned int row_position;
} acr_destroy;

typedef struct acr_grid {
  unsigned int grid_size;
} acr_grid;

struct acr_parameter_specifier {
  unsigned int pointer_depth;
  char* type_name;
  bool is_pointer;
};

typedef struct acr_parameter_declaration {
  unsigned int num_specifiers;
  struct acr_parameter_specifier* parameter_specifiers;
  char* parameter_name;
} acr_parameter_declaration;

typedef struct acr_init {
  unsigned int row_position;
  char* function_name;
  unsigned int num_parameters;
  acr_parameter_declaration* parameters;
} acr_init;

struct acr_array_declaration {
  unsigned int num_specifiers;
  struct acr_parameter_specifier* parameter_specifiers;
  char* parameter_name;
  unsigned int num_array_dimensions;
  unsigned int* array_dimensions;
};

enum acr_monitor_processing_funtion {
  min,
  max,
};

typedef struct acr_monitor {
  struct acr_array_declaration data_monitored;
  enum acr_monitor_processing_funtion monitor_function;
  char* filter_name;
} acr_monitor;

enum acr_strategy_type {
  direct,
  range,
};

enum acr_strategy_value_type {
  integer,
  floating_point,
};

typedef struct acr_strategy {
  enum acr_strategy_type strategy_type;
  enum acr_strategy_value_type value_type;
  union val{
    int value;
    float value;
  } boundaries[2];
  char* strategy_name;
} acr_strategy;

enum acr_type {
  acr_alternative,
  acr_destroy,
  acr_grid,
  acr_init,
  acr_monitor,
  acr_strategy,
};

typedef struct acr_option {
  enum acr_type type;
  union options {
    acr_alternative alternative;
    acr_destroy destroy;
    acr_grid grid;
    acr_init init;
    acr_monitor monitor;
    acr_strategy strategy;
  };
} acr_option;

typedef struct acr_pragma {
  acr_option* node;
  struct acr_pragma* next;
} acr_pragma;


static inline enum acr_type acr_get_type(acr_option* option) {
  return option->type;
}

acr_option* acr_new_alternative_parameter(const char* alternative_name,
                                          const char* function_to_swap
                                          const char* replacement_function);

acr_option* acr_new_alternative_function(const char* alternative_name,
                                         const char* parameter_to_swap,
                                         int replacement_value);

static inline enum acr_alternative_type acr_alternative_get_type(
    acr_option* option) {
  return option->options.alternative.type;
}

static inline char* acr_alternative_get_alternative_name(acr_option* option) {
  return option->options.alternative.alternative_name;
}

static inline char* acr_alternative_get_alternative_name(acr_option* option) {
  return option->options.alternative.alternative_name;
}

static inline char* acr_alternative_get_object_to_swap_name(
    acr_option * option){
  return option->options.alternative.name_of_object_to_swap;
}

static inline char* acr_alternative_get_remplacement_function(
    acr_option* option){
  return acr_alternative_get_type(option) == acr_alternative_function ?
    option->options.alternative.swapped_by.replacement_function :
    NULL;
}

static inline int acr_alternative_get_remplacement_parameter(
    acr_option* option){
  return acr_alternative_get_type(option) == acr_alternative_function ?
    option->options.alternative.swapped_by.replacement_value :
    0;
}

#endif // __PRAGMA_STRUCT_H
