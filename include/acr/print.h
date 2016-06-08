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

/**
 * \brief Print an acr option
 * \param[in,out] out The output stream
 * \param[in] option The option to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_option(FILE* out, acr_option option, size_t indent_level);

/**
 * \brief Print an acr alternative
 * \param[in,out] out The output stream
 * \param[in] alternative The alternative to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_alternative(FILE* out, acr_option alternative,
    size_t indent_level);

/**
 * \brief Print an acr destroy
 * \param[in,out] out The output stream
 * \param[in] destroy The destroy construct to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_destroy(FILE* out, acr_option destroy, size_t indent_level);

/**
 * \brief Print an acr grid
 * \param[in,out] out The output stream
 * \param[in] grid The grid to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_grid(FILE* out, acr_option grid, size_t indent_level);

/**
 * \brief Print an acr init
 * \param[in,out] out The output stream
 * \param[in] init The init to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_init(FILE* out, acr_option init, size_t indent_level);

/**
 * \brief Print an acr monitor
 * \param[in,out] out The output stream
 * \param[in] monitor The monitor to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_monitor(FILE* out, acr_option monitor, size_t indent_level);

/**
 * \brief Print an acr strategy
 * \param[in,out] out The output stream
 * \param[in] strategy The strategy to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_strategy(FILE* out, acr_option strategy, size_t indent_level);

/**
 * \brief Print an acr declaration list
 * \param[in,out] out The output stream
 * \param[in] num_declarations The number of declarations
 * \param[in] declaration_list The declaration list
 */
void pprint_acr_parameter_declaration_list(FILE* out,
                                      size_t num_declarations,
                                   acr_parameter_declaration* declaration_list);

/**
 * \brief Print an acr parameter specifier list
 * \param[in,out] out The output stream
 * \param[in] num_specifiers The number of parameter specifiers
 * \param[in] specifier_list The parameter specifier list
 */
void pprint_acr_parameter_specifier_list(FILE* out,
                                      size_t num_specifiers,
                                      acr_parameter_specifier* specifier_list);

/**
 * \brief Print an acr array declaration
 * \param[in,out] out The output stream
 * \param[in] declaration The array declaration
 */
void pprint_acr_array_declaration(FILE* out,
                                  acr_array_declaration* declaration);

/**
 * \brief Print an acr compute node
 * \param[in,out] out The output stream
 * \param[in] node The compute node to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_compute_node(FILE* out, acr_compute_node node,
    size_t indent_level);

/**
 * \brief Print an acr compute node list
 * \param[in,out] out The output stream
 * \param[in] node_list The compute node list to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_compute_node_list(FILE* out,
                                  acr_compute_node_list node_list,
                                  size_t indent_level);

/**
 * \brief Print an acr array_dimension
 * \param[in,out] out The output stream
 * \param[in] dim The array dimension
 * \param[in] print_braces Print braces between the value if true
 */
void print_acr_array_dimensions(FILE* out,
    acr_array_dimension dim, bool print_braces);

/**
 * \brief Print an acr deffered destroy
 * \param[in,out] out The output stream
 * \param[in] option The deffered destroy to print
 * \param[in] indent_level The indentation level to print
 */
void pprint_acr_deffered_destroy(FILE *out, acr_option option,
    size_t indent_level);

/**
 * \brief Print an acr option list
 * \param[in,out] out The output stream
 * \param[in] opt_list The acr option list
 * \param[in] list_size The acr option list size
 * \param[in] indent_level The indentation level to print
 */
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
