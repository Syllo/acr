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
 * \file acr_runtime.h
 * \brief This is the master include file for runtime purpose
 *
 * Just include this file and you will have access to all ACR runtime
 * declarations.
 *
 * \defgroup acr_runtime ACR runtime library
 *
 * @{
 * \brief All the required functions and structures needed at runtime
 *
 * \defgroup runtime_data Runtime data structures
 * \defgroup runtime_perf Runtime self performance tests
 * \defgroup runtime_threads Runtime threads
 * \defgroup runtime_generation Runtime code generation
 * \defgroup runtime_stats Runtime kernel statistics
 * \defgroup runtime_osl Runtime OpenScop helper functions
 * \defgroup runtime_verify Runtime grid comparison function
 * \defgroup runtime_alternative Runtime alternative structure
 *
 * @}
 *
 */

#ifndef __ACR_RUNTIME_H
#define __ACR_RUNTIME_H

#define _POSIX_C_SOURCE 200809L
#include <acr/acr_runtime_code_generation.h>
#include <acr/acr_runtime_data.h>
#include <acr/acr_runtime_threads.h>
#include <acr/acr_stats.h>
#include <stdatomic.h>

#define __ACR_PRESENT__

#endif // __ACR_RUNTIME_H
