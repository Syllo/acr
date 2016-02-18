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

#ifndef __ACR_GENCODE_H
#define __ACR_GENCODE_H

#include <stdio.h>

#include "acr/pragma_struct.h"

void acr_generate_code(const char* filename);

void acr_print_structure_and_related_scop(FILE* out, const char* filename);

void acr_generate_preamble(FILE* out, const char* );

void acr_print_node_initialization(FILE* out, const acr_compute_node node);

void acr_print_init_function_declaration(FILE* out, const acr_option init);

#endif // __ACR_GENCODE_H
