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

#ifndef __ACR_RUNTIME_STRATEGIES__H
#define __ACR_RUNTIME_STRATEGIES__H

typedef unsigned char *acr_monitored_data;

enum acr_runtime_alternative_type {
  acr_runtime_alternative_parameter,
  acr_runtime_alternative_function,
};

struct runtime_alternative {
  enum acr_runtime_alternative_type type;
  union {
    struct {
      char *parameter_name;
      long parameter_value;
    } parameter;
    char *function;
  } value;
};

typedef struct runtime_alternative *runtime_alternative;

enum acr_runtime_strategy_type {
  acr_runtime_strategy_direct,
  acr_runtime_strategy_range,
};

struct runtime_strategies {
  enum acr_runtime_strategy_type type;
  unsigned char strategy_value_min;
  unsigned char strategy_value_max;
  runtime_alternative *corresponding_alternative;
};

typedef struct {
  struct runtime_strategies *strategies;
  size_t num_strategies;
} *runtime_strategies;

runtime_alternative acr_runtime_get_alternative_from_val(
    size_t position,
    acr_monitored_data data,
    runtime_strategies strategies);

#endif // __ACR_RUNTIME_STRATEGIES__H
