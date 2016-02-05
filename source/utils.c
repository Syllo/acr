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

#include "acr/utils.h"

#include <stdlib.h>
#include <string.h>

char* acr_strdup(const char* to_duplicate) {
  size_t string_size = strlen(to_duplicate);
  char* copy = malloc(string_size+1 * sizeof(*copy));
  for (size_t i = 0; i < string_size ; ++i) {
    copy[i] = to_duplicate[i];
  }
  copy[string_size] = '\0';
  return copy;
}