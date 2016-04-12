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

#ifndef __ACR_PARSER_UTILS_H
#define __ACR_PARSER_UTILS_H

#include "acr/pragma_struct.h"

struct parser_option_list {
  acr_option option;
  struct parser_option_list* next;
  struct parser_option_list* previous;
};

struct acr_pragma_parser_name_error_utils {
  const char* name;
  const char* error_message;
};

static const struct acr_pragma_parser_name_error_utils
    acr_pragma_alternative_names[] =
  {
    [acr_alternative_parameter] = {"parameter", "[ACR] Hint: did you want to"
                              " create an alternative parameter construct?\n"},
    [acr_alternative_function]  = {"function",  "[ACR] Hint: did you want to"
                              " create an alternative parameter construct?\n"},
  };

static const struct acr_pragma_parser_name_error_utils
    acr_pragma_strategy_names[] =
  {
    [acr_strategy_direct] = {"direct",  "[ACR] Hint: did you want to create a"
                                        " direct construct?\n"},
    [acr_strategy_range]  = {"range",  "[ACR] Hint: did you want to create a"
                                        " range construct?\n"},
  };

struct array_dimensions_list {
  acr_array_dimension dim;
  struct array_dimensions_list* next;
};

struct parameter_declaration {
  struct parameter_declaration* next;
  struct parameter_declaration* previous;
  char* name;
  size_t pointer_depth;
};

struct parameter_declaration_list {
  struct parameter_declaration* declaration;
  struct parameter_declaration_list* next;
  struct parameter_declaration_list* previous;
};

struct parameter_declaration* add_param_declaration(
    struct parameter_declaration* current, char* name,
    size_t pointer_depth);

struct parameter_declaration_list* add_declaration_to_list(
    struct parameter_declaration_list* current,
    struct parameter_declaration* declaration);

size_t translate_and_free_param_declaration_list(
    struct parameter_declaration_list* list,
    acr_parameter_declaration** list_to_initialize);

size_t get_name_and_specifiers_and_free_parameter_declaration(
    struct parameter_declaration* declaration,
    char** parameter_name,
    acr_parameter_specifier** specifier_list);

static inline void free_param_declarations(struct parameter_declaration* dec) {
  while(dec && dec->next) {
    dec = dec->next;
    free(dec->previous->name);
    free(dec->previous);
  }
  free(dec->name);
  free(dec);
}

static inline void free_param_decl_list(struct parameter_declaration_list* dec) {
  while(dec && dec->next) {
    dec = dec->next;
    free_param_declarations(dec->previous->declaration);
    free(dec->previous);
  }
  free_param_declarations(dec->declaration);
  free(dec);
}

static inline struct parser_option_list* parser_option_list_add(
    acr_option option,
    struct parser_option_list* option_list) {
  struct parser_option_list* list = malloc(sizeof(*list));
  list->next = option_list;
  list->previous = NULL;
  if (option_list)
    option_list->previous = list;
  list->option = option;
  return list;
}

static inline size_t parser_translate_option_list_and_free(
    struct parser_option_list* old_list,
    acr_option_list* new_list) {
  if(!old_list) {
    *new_list = NULL;
    return 0;
  }
  size_t size_list = 1ul;
  while (old_list->next) {
    old_list = old_list->next;
    ++size_list;
  }

  *new_list = acr_new_option_list(size_list);
  for (size_t i = 0; i < size_list; ++i) {
    acr_option_list_set_option(old_list->option, i, *new_list);
    if (old_list->previous) {
      old_list = old_list->previous;
      free(old_list->next);
    }
  }
  free(old_list);
  return size_list;
}

static inline void parser_free_option_list(struct parser_option_list* list) {
  if (!list)
    return;
  while (list->next) {
    list = list->next;
    acr_free_option(list->previous->option);
    free(list->previous);
  }
  acr_free_option(list->option);
  free(list);
}

static inline struct array_dimensions_list* new_array_dim_list(
    acr_array_dimension dim) {
  struct array_dimensions_list* new_list = malloc(sizeof(*new_list));
  new_list->next = NULL;
  new_list->dim = dim;
  return new_list;
}

static inline void free_array_dim_list(struct array_dimensions_list* list) {
  if (!list)
    return;
  struct array_dimensions_list* next = list->next;
  do {
    next = list->next;
    acr_free_array_dimension(list->dim);
    free(list);
    list = next;
  }while (list);
}

size_t array_dim_list_size_free_convert(
    struct array_dimensions_list* constructed_list,
    acr_array_dimensions_list* list);

#endif // __ACR_PARSER_UTILS_H
