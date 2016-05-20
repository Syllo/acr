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

/**
 *
 * \file print.h
 * \brief Print the pragma data structure
 *
 * \defgroup build_print
 *
 * @{
 * \brief Print the pragma construct data structure
 *
 */

#ifndef __ACR_PRINT_H
#define __ACR_PRINT_H

#include <stdint.h>
#include <stdio.h>

#include "acr/pragma_struct.h"

void pprint_acr_option(FILE* out, acr_option option, size_t indent_level);

void pprint_acr_alternative(FILE* out, acr_option alternative,
    size_t indent_level);

void pprint_acr_destroy(FILE* out, acr_option destroy, size_t indent_level);

void pprint_acr_grid(FILE* out, acr_option grid, size_t indent_level);

void pprint_acr_init(FILE* out, acr_option init, size_t indent_level);

void pprint_acr_monitor(FILE* out, acr_option monitor, size_t indent_level);

void pprint_acr_strategy(FILE* out, acr_option strategy, size_t indent_level);

void pprint_acr_parameter_declaration_list(FILE* out,
                                      size_t num_declarations,
                                   acr_parameter_declaration* declaration_list);

void pprint_acr_parameter_specifier_list(FILE* out,
                                      size_t num_specifiers,
                                      acr_parameter_specifier* specifier_list);

void pprint_acr_array_declaration(FILE* out,
                                  acr_array_declaration* declaration);

void pprint_acr_compute_node(FILE* out, acr_compute_node node,
    size_t indent_level);

void pprint_acr_compute_node_list(FILE* out,
                                  acr_compute_node_list node_list,
                                  size_t indent_level);

void print_acr_array_dimensions(FILE* out,
    acr_array_dimension dim, bool print_braces);

void pprint_acr_deffered_destroy(FILE *out, acr_option option,
    size_t indent_level);

void pprint_acr_option_list(FILE *out,
                            acr_option_list opt_list,
                            size_t list_size,
                            size_t indent_level);

#endif // __ACR_PRINT_H

/**
 *
 * @}
 *
 */
