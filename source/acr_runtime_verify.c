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

#include "acr/acr_runtime_verify.h"

#include <string.h>

bool acr_verify_me(size_t size_buffers,
    unsigned char const*const restrict current,
    unsigned char const*const restrict more_recent) {
  bool same = true;
#pragma omp simd reduction(&&:same)
  for(size_t i = 0; i < size_buffers; i++) {
    same = same && (current[i] <= more_recent[i]);
  }
  return same;
}

void acr_verify_versionning(size_t size_buffers,
    unsigned char const*const restrict current,
    unsigned char const*const restrict more_recent,
    unsigned char *restrict maximized_version,
    size_t num_alternatives,
    double *delta,
    bool *still_valid) {
  size_t total_difference = 0;
  bool still_valid_local = true;

  for(size_t i = 0; i < size_buffers; i++) {
    if (more_recent[i] < current[i]) {
      maximized_version[i] = more_recent[i];
      total_difference = total_difference + current[i] - more_recent[i];
      still_valid_local = false;
    } else {
      maximized_version[i] = current[i];
      total_difference = total_difference + more_recent[i] - current[i];
    }
  }
  size_buffers *= num_alternatives;
  *delta = (double) total_difference / (double) size_buffers;
  *still_valid = still_valid_local;
}
