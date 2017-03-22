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

/**
 * \brief Verify the validity of the current grid against compiled version grid.
 * \param[in] size_buffers The size of the grid buffer
 * \param[in] current The grid currently used as the kernel
 * \param[in] more_recent The more recent grid generated by the monitoring
 * function
 * \retval true The current grid is still valid
 * \retval false Otherwise
 */
bool acr_verify_me(size_t size_buffers,
    unsigned char const* restrict current,
    unsigned char const* restrict more_recent);

/**
 * \brief Verification function suited for versioning
 * \param[in] size_buffers The size of the grid buffer
 * \param[in] current The grid currently used as the kernel
 * \param[in] more_recent The more recent grid generated by the monitoring
 * \param[out] maximized_version The grid that will store the new maximized
 * version
 * \param[in] num_alternatives The number of alternatives
 * \param[out] delta The computed difference between the more recent and the
 * current version
 * \param[out] still_valid True if the current grid is still valid, false
 * otherwise
 */
void acr_verify_versioning(size_t size_buffers,
    unsigned char const* current,
    unsigned char const* more_recent,
    unsigned char *maximized_version,
    size_t num_alternatives,
    double *delta,
    bool *still_valid);

/**
 * \brief Verification function suited for stencil
 * \param[in] dims_size The size of each of the two dimensions
 * \param[in] current_untouched The grid row monitoring grid used by the kernel
 * \param[in] more_recent The more recent grid generated by the monitoring
 * \param[in] current_optimized_version The optimized version used by the kernel
 * \param[out] new_optimized_version The new optimized version
 * \param[out] required_compilation True if the new optimized version is more
 * precise than the current one.
 * \param[out] still_valid True if the current grid is still valid, false
 * otherwise
 *
 *  This version applies the Moore 2D stencil at distance 1. So we try to
 *  anticipate change for nearby cells by looking at the neighbourhood and
 *  over-approximate the cell precision needs.
 *
 */
void acr_verify_2dstencil(
    unsigned char max_alt,
    unsigned long const dims_size[2],
    unsigned char const* more_recent,
    unsigned char const* current_optimized_version,
    unsigned char *new_optimized_version,
    bool *required_compilation,
    bool *still_valid);

#endif // __ACR_RUNTIME_VERIFY_H

/**
 *
 * @}
 *
 */
