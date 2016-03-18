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

#ifndef __ACR_RUNTIME_ALTERNATIVES__H
#define __ACR_RUNTIME_ALTERNATIVES__H

#include <isl/set.h>

enum acr_runtime_alternative_type {
  acr_runtime_alternative_parameter,
  acr_runtime_alternative_function,
};

struct runtime_alternative {
  enum acr_runtime_alternative_type type;
  unsigned long alternative_number;
  struct {
    union {
      char *function_to_swap;
      struct {
        unsigned long parameter_position;
        long parameter_value;
        unsigned long num_domains;
        isl_set **parameter_constraints;
        struct {
          unsigned long num_free_dim;
          unsigned long *free_dim_position;
        } free_dims;
      } parameter;
    } alt;
    char *name_to_swap;
  } value;
};

#endif // __ACR_RUNTIME_ALTERNATIVES__H
