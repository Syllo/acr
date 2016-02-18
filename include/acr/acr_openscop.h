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

#ifndef __ACR_OPENSCOP_H
#define __ACR_OPENSCOP_H

#include "acr/pragma_struct.h"

#include <osl/scop.h>
#include <clan/scop.h>

osl_scop_p acr_extract_scop_in_compute_node(const acr_compute_node node,
                                            FILE* input_file,
                                            const char* name_input_file);

void acr_print_scop_to_buffer(osl_scop_p scop, char** buffer,
    size_t* size_buffer);

osl_scop_p acr_read_scop_from_buffer(char* buffer, size_t size_buffer);

void acr_get_start_and_stop_for_clan(const acr_compute_node node,
    unsigned long* start, unsigned long *stop);

void acr_scop_get_coordinates_start_end_kernel(
    const acr_compute_node compute_node,
    const osl_scop_p scop,
    unsigned long* start, unsigned long* end);

#endif // __ACR_OPENSCOP_H
