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

void acr_verify_versioning(size_t size_buffers,
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
      // Overapprox if recently added
      maximized_version[i] = more_recent[i];
      if (maximized_version[i] > 0)
        maximized_version[i] = (unsigned char) (maximized_version[i] - 1);
      total_difference += current[i] - more_recent[i] ? 1 : 0;
      still_valid_local = false;
    } else {
      maximized_version[i] = current[i];
      total_difference += more_recent[i] - current[i] ? 1 : 0;
    }
  }
  (void) num_alternatives;
  *delta = (double) total_difference / (double) size_buffers;
  *still_valid = still_valid_local;
}

// Von Neumann neighborhood

void acr_verify_2dstencil(
    unsigned long const dims_size[2],
    unsigned char const*const restrict current_untouched,
    unsigned char const*const restrict more_recent,
    unsigned char const*const restrict current_optimized_version,
    unsigned char *restrict new_optimized_version,
    bool *required_compilation,
    bool *still_valid) {
  bool still_valid_local = true, required_compilation_local = false;

  const size_t isize = dims_size[0], jsize = dims_size[1];
/*#pragma omp parallel*/
  {

/*#pragma omp for schedule(static,1) reduction(&&:still_valid_local) \*/
    /*reduction(&&:required_compilation_local)*/
    for(size_t i = 0; i < isize; ++i) {
      size_t element_position_i = i*jsize;
      for(size_t j = 0; j < jsize; ++j) {
        size_t upper, lower, left, right;
        const size_t element_position_2d = element_position_i + j;
        upper = element_position_2d - jsize;
        lower = element_position_2d + jsize;
        left = element_position_2d - 1;
        right = element_position_2d + 1;

        if (current_untouched[element_position_2d] >
            more_recent[element_position_2d]) {
          new_optimized_version[element_position_2d] =
            more_recent[element_position_2d];
          if (current_optimized_version[element_position_2d] > more_recent[element_position_2d])
            still_valid_local = false;
        } else {
          new_optimized_version[element_position_2d] =
            current_optimized_version[element_position_2d];
        }

        if (i > 0 && i < isize -1) {
          if (j > 0 && j < jsize - 1) {
            if (current_untouched[upper] > more_recent[upper] &&
                more_recent[upper] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[upper];
              required_compilation_local = true;
            }
            if (current_untouched[lower] > more_recent[lower] &&
                more_recent[lower] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[lower];
              required_compilation_local = true;
            }
            if (current_untouched[left] > more_recent[left] &&
                more_recent[left] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[left];
              required_compilation_local = true;
            }
            if (current_untouched[right] > more_recent[right] &&
                more_recent[right] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[right];
              required_compilation_local = true;
            }
          } else if (j == 0) {
            if (current_untouched[upper] > more_recent[upper] &&
                more_recent[upper] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[upper];
              required_compilation_local = true;
            }
            if (current_untouched[lower] > more_recent[lower] &&
                more_recent[lower] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[lower];
              required_compilation_local = true;
            }
            if (current_untouched[right] > more_recent[right] &&
                more_recent[right] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[right];
              required_compilation_local = true;
            }
          } else { // j == jsize - 1
            if (current_untouched[upper] > more_recent[upper] &&
                more_recent[upper] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[upper];
              required_compilation_local = true;
            }
            if (current_untouched[lower] > more_recent[lower] &&
                more_recent[lower] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[lower];
              required_compilation_local = true;
            }
            if (current_untouched[left] > more_recent[left] &&
                more_recent[left] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[left];
              required_compilation_local = true;
            }
          }
        } else if (i == 0) {
          if (j > 0 && j < jsize - 1) {
            if (current_untouched[lower] > more_recent[lower] &&
                more_recent[lower] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[lower];
              required_compilation_local = true;
            }
            if (current_untouched[left] > more_recent[left] &&
                more_recent[left] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[left];
              required_compilation_local = true;
            }
            if (current_untouched[right] > more_recent[right] &&
                more_recent[right] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[right];
              required_compilation_local = true;
            }
          } else if (j == 0) {
            if (current_untouched[lower] > more_recent[lower] &&
                more_recent[lower] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[lower];
              required_compilation_local = true;
            }
            if (current_untouched[right] > more_recent[right] &&
                more_recent[right] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[right];
              required_compilation_local = true;
            }
          } else { // j == jsize - 1
            if (current_untouched[lower] > more_recent[lower] &&
                more_recent[lower] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[lower];
              required_compilation_local = true;
            }
            if (current_untouched[left] > more_recent[left] &&
                more_recent[left] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[left];
              required_compilation_local = true;
            }
          }
        } else { // i == isize - 1
          if (j > 0 && j < jsize - 1) {
            if (current_untouched[upper] > more_recent[upper] &&
                more_recent[upper] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[upper];
              required_compilation_local = true;
            }
            if (current_untouched[left] > more_recent[left] &&
                more_recent[left] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[left];
              required_compilation_local = true;
            }
            if (current_untouched[right] > more_recent[right] &&
                more_recent[right] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[right];
              required_compilation_local = true;
            }
          } else if (j == 0) {
            if (current_untouched[upper] > more_recent[upper] &&
                more_recent[upper] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[upper];
              required_compilation_local = true;
            }
            if (current_untouched[right] > more_recent[right] &&
                more_recent[right] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[right];
              required_compilation_local = true;
            }
          } else { // j == jsize - 1
            if (current_untouched[upper] > more_recent[upper] &&
                more_recent[upper] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[upper];
              required_compilation_local = true;
            }
            if (current_untouched[left] > more_recent[left] &&
                more_recent[left] < new_optimized_version[element_position_2d]) {
              new_optimized_version[element_position_2d] = more_recent[left];
              required_compilation_local = true;
            }
          }
        }
      }
    }
  }
  *still_valid = still_valid_local;
  *required_compilation = required_compilation_local;
}
