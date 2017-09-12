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


static inline unsigned char get_min_8_val(
    unsigned char a, unsigned char b, unsigned char c, unsigned char d,
    unsigned char e, unsigned char f, unsigned char g, unsigned char h) {
  unsigned char min = a > b ? b : a;
  min = min > c ? c : min;
  min = min > d ? d : min;
  min = min > e ? e : min;
  min = min > f ? f : min;
  min = min > g ? g : min;
  min = min > h ? h : min;
  return min;
}

// Moore neighbourhood
#include <stdio.h>

void acr_verify_2dstencil(
    unsigned char max_alt,
    unsigned long const dims_size[2],
    unsigned char const*const restrict more_recent,
    unsigned char const*const restrict current_optimized_version,
    unsigned char *restrict new_optimized_version,
    bool *required_compilation,
    bool *still_valid) {

  bool still_valid_local = true, required_compilation_local = false;

  const size_t isize = dims_size[0], jsize = dims_size[1];
  size_t too_much_precision = 0;

  for(size_t i = 0; i < isize; ++i) {
    size_t element_position_i = i*jsize;
    for(size_t j = 0; j < jsize; ++j) {
      size_t n, s, e, w, ne, nw, se, sw;
      const size_t element_position_2d = element_position_i + j;
      n  = element_position_2d - jsize;
      ne = n + 1;
      nw = n - 1;
      s  = element_position_2d + jsize;
      se = s + 1;
      sw = s - 1;
      w  = element_position_2d - 1;
      e  = element_position_2d + 1;
      unsigned char
        n_val = i == 0 ? 255 : more_recent[n],
        s_val = i == (isize-1) ? 255 : more_recent[s],
        w_val = j == 0 ? 255 : more_recent[w],
        e_val = j == (jsize-1) ? 255 : more_recent[e],
        se_val = (i == (isize-1) || j == (jsize-1)) ? 255 : more_recent[se],
        sw_val = (i == (isize-1) || j == 0) ? 255 : more_recent[sw],
        ne_val = (i == 0 || j == (jsize-1)) ? 255 : more_recent[ne],
        nw_val = (i == 0 || j == 0) ? 255 : more_recent[nw];

      unsigned char min = get_min_8_val(
          n_val, ne_val, nw_val, s_val, se_val, sw_val, e_val, w_val);
      if (min < max_alt)
        min ++;
      if (min > more_recent[element_position_2d]) {
        if (more_recent[element_position_2d] < current_optimized_version[element_position_2d]) {
          still_valid_local = false;
        }
        new_optimized_version[element_position_2d] = more_recent[element_position_2d];
      } else {
        if (min < current_optimized_version[element_position_2d]) {
          required_compilation_local = true;
        } else {
          if (min > current_optimized_version[element_position_2d]) {
            too_much_precision += 1;
          }
        }
        new_optimized_version[element_position_2d] = min;
      }
    }
  }
  size_t total_computation = isize * jsize;
  if ((double) too_much_precision / (double)total_computation > 0.15) {
    /*fprintf(stderr, "Compile because too much diff\n");*/
    required_compilation_local = true;
  }

  *still_valid = still_valid_local;
  *required_compilation = required_compilation_local;
}
