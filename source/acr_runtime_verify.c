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

bool acr_verify_me(size_t size_buffers,
    const unsigned char *current,
    const unsigned char *more_precise) {
  size_t i;
  bool same = true;
//#pragma omp parallel for reduction(&&:same)
  for(i = 0; i < size_buffers; i++) {
    same = same && (current[i] >= more_precise[i]);
  }
  return same;
}
