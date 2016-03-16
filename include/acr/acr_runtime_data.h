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

#ifndef __ACR_RUNTIME_DATA_H
#define __ACR_RUNTIME_DATA_H

#include <cloog/cloog.h>
#include <acr/acr_openscop.h>
#include <acr/runtime_alternatives.h>

struct acr_runtime_data {
  CloogInput* cloog_input;
  CloogState* state;
  struct osl_scop* osl_relation;
  unsigned long num_alternatives;
  struct runtime_alternative *alternatives;
  unsigned long num_parameters;
  unsigned long num_dimensions;
  size_t *monitor_dimensions;
  size_t monitor_total_size;
  unsigned long grid_size;
};

void init_acr_runtime_data(
    struct acr_runtime_data* data,
    char *scop,
    size_t scop_size);

void free_acr_runtime_data(struct acr_runtime_data* data);

#endif // __ACR_RUNTIME_DATA_H
