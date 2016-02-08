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

static const char* acr_pragma_options_error_messages[] =
  {
    [acr_type_alternative] = "[ACR] Hint: take a look at the"
                             " alternative construct\n",
    [acr_type_destroy]     = "[ACR] Hint: take a look at the"
                             " destroy construct\n",
    [acr_type_grid]        = "[ACR] Hint: take a look at the"
                             " grid construct\n",
    [acr_type_init]        = "[ACR] Hint: take a look at the"
                             " init construct\n",
    [acr_type_monitor]     = "[ACR] Hint: take a look at the"
                             " monitor construct\n",
    [acr_type_strategy]    = "[ACR] Hint: take a look at the"
                             " strategy construct\n",
    [acr_type_unknown]     = "[ACR] Warning: unrecognized or"
                             " malformed pragma\n",
  };

static const char* acr_pragma_processing_functions[] =
  {
    [acr_monitor_function_min]  = "min",
    [acr_monitor_function_max]  = "max",
  };

static const struct acr_pragma_parser_name_error_utils
    acr_pragma_strategy_names[] =
  {
    [acr_strategy_direct] = {"direct",  "[ACR] Hint: did you want to create a"
                                        " direct construct?\n"},
    [acr_strategy_range]  = {"range",  "[ACR] Hint: did you want to create a"
                                        " range construct?\n"},
  };

struct parameter_declaration {
  struct parameter_declaration* next;
  struct parameter_declaration* previous;
  char* name;
  unsigned int pointer_depth;
};

struct parameter_declaration_list {
  struct parameter_declaration* declaration;
  struct parameter_declaration_list* next;
  struct parameter_declaration_list* previous;
};

struct parameter_declaration* add_param_declaration(
    struct parameter_declaration* current, char* name,
    unsigned int pointer_depth);

struct parameter_declaration_list* add_declaration_to_list(
    struct parameter_declaration_list* current,
    struct parameter_declaration* declaration);

unsigned int translate_and_free_param_declaration_list(
    struct parameter_declaration_list* list,
    acr_parameter_declaration** list_to_initialize);

unsigned int get_name_and_specifiers_and_free_parameter_declaration(
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

#endif // __ACR_PARSER_UTILS_H
