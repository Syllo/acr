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
 * \file acr_runtime_osl.h
 * \brief OpenScop helper functions
 *
 * \defgroup runtime_osl
 *
 * @{
 * \brief Openscop helper functions
 *
 */

#ifndef __ACR_OSL_RUNTIME_H
#define __ACR_OSL_RUNTIME_H

#include <osl/scop.h>

osl_scop_p acr_read_scop_from_buffer(char* buffer, size_t size_buffer);

#endif // __ACR_OSL_RUNTIME_H

/**
 *
 * @}
 *
 */
