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

#ifndef __ACR_RUNTIME_THREADS_H
#define __ACR_RUNTIME_THREADS_H

#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

void* acr_runtime_monitoring_function(void* in_data);

void* acr_runtime_compile_thread(void* in_data);

void* acr_cloog_generate_code_from_alt(void* in_data);

void* acr_verification_and_coordinator_function(void *in_data);

#endif // __ACR_RUNTIME_THREADS_H
