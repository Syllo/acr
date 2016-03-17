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

#ifndef __ACR_ISL_RUNTIME_H
#define __ACR_ISL_RUNTIME_H

#include <assert.h>
#include <isl/set.h>

#include "acr/runtime_alternatives.h"

isl_set** acr_isl_set_from_monitor(
    isl_ctx *ctx,
    const unsigned char *data,
    unsigned long num_strategies,
    unsigned long int num_param,
    unsigned long num_dimensions,
    const size_t *dimensions,
    size_t dimensions_total_size,
    unsigned long tiling_size,
    struct runtime_alternative*
        (*get_alternative_from_val)(unsigned char data));

isl_set* isl_set_from_alternative_parameter_construct(
    isl_ctx *ctx,
    unsigned long num_parameters,
    unsigned long num_dimensions,
    struct runtime_alternative* alternative);

#endif // __ACR_ISL_RUNTIME_H
