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
 * \file acr_runtime_threads.h
 * \brief Function to call with pthread in order to start the runtime threads.
 *
 * \defgroup runtime_threads
 *
 * @{
 * \brief The threading system of ACR
 *
 */


#ifndef __ACR_RUNTIME_THREADS_H
#define __ACR_RUNTIME_THREADS_H

/**
 * \brief Thread function that you must call with pthread to start the
 * coordinator
 * \param[in] in_data A struct ::acr_runtime_data cast in (void*).
 * \retval NULL
 */
void* acr_verification_and_coordinator_function(void *in_data);

/**
 * \brief The kernel used when doing performance tests
 * \param[in] in_data A struct ::acr_runtime_perf cast in (void*)
 * \retval NULL
 */
void* acr_runtime_perf_compile_time_zero(void* in_data);

#endif // __ACR_RUNTIME_THREADS_H

/**
 *
 * @}
 *
 */
