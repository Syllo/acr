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

#ifndef __ACR_RUNTIME_PERF_H
#define __ACR_RUNTIME_PERF_H

#include <stdint.h>

#include "acr/acr_runtime_data.h"

struct acr_runtime_perf {
  struct acr_runtime_data *rdata;
  struct acr_performance_list *list_head;
  struct acr_performance_list *compilation_list;
};

void* acr_runntime_perf_end_step(struct acr_runtime_perf *perf);

void acr_runtime_perf_clean(struct acr_runtime_perf *perf);

void acr_runtime_print_perf_function_call(
    FILE* output,
    struct acr_runtime_perf *perf);

#endif // __ACR_RUNTIME_PERF_H
