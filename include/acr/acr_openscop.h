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

void acr_print_scop_to_buffer(osl_scop_p scop, char** buffer,
    size_t* size_buffer);

osl_scop_p acr_read_scop_from_buffer(char* buffer, size_t size_buffer);

osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const osl_scop_p scop);

osl_strings_p acr_openscop_get_monitor_parameters(const acr_option monitor);

#endif // __ACR_OPENSCOP_H
