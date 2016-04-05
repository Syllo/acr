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

#include "acr/acr_runtime_threads.h"

void* acr_runtime_monitoring_function(void* in_data) {
  struct acr_runtime_threads_monitor_init *init_data =
    (struct acr_runtime_threads_monitor_init*) in_data;
  while (1) {
    pthread_mutex_lock(init_data->waiting_when_finished);
    *init_data->waiting = true;
    while (init_data->waiting)
      pthread_cond_wait(init_data->waiting_cond, init_data->waiting_when_finished);
    pthread_mutex_unlock(init_data->waiting_when_finished);

    unsigned char* new_result =
      malloc(init_data->rdata->monitor_total_size * sizeof(*new_result));
    init_data->monitor_function(new_result);
    struct acr_runtime_threads_compile_data d;
  }
  pthread_exit(NULL);
}

void* acr_runtime_compile_thread(void* in_data) {
  struct acr_runtime_threads_compile_data *input_data =
    (struct acr_runtime_threads_compile_data *) in_data;
  pthread_exit(NULL);
}
