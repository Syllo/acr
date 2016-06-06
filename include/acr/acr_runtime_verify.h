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
 * \file acr_runtime_verify.h
 * \brief Grid comparison functions
 *
 * \defgroup runtime_verify
 *
 * @{
 * \brief Comparison between the current and previously used grid data
 *
 */

#ifndef __ACR_RUNTIME_VERIFY_H
#define __ACR_RUNTIME_VERIFY_H

#include <stdbool.h>
#include <stdlib.h>

bool acr_verify_me(size_t size_buffers,
    const unsigned char *current,
    const unsigned char *more_precise);

void acr_verify_versionning(size_t size_buffers,
    unsigned char const*const current,
    unsigned char const*const more_recent,
    unsigned char *maximized_version,
    size_t num_alternatives,
    double *delta,
    bool *still_valid);

void acr_verify_2dstencil(
    unsigned long const* dims_size,
    unsigned char const*const restrict current_untouched,
    unsigned char const*const restrict more_recent,
    unsigned char const*const restrict current_optimized_version,
    unsigned char *restrict new_optimized_version,
    bool *required_compilation,
    bool *still_valid);

#endif // __ACR_RUNTIME_VERIFY_H

/**
 *
 * @}
 *
 */
