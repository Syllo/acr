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

#include "acr/pragma_struct.h"

#include <string.h>
#include <acr/print.h>

acr_option acr_new_alternative_function(const char* alternative_name,
                                        const char* function_to_swap,
                                        const char* replacement_function,
                                        size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->options.alternative.pragma_position = pragma_position;
  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_function;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  option->options.alternative.name_of_object_to_swap =
    acr_strdup(function_to_swap);
  option->options.alternative.swapped_by.replacement_function =
    acr_strdup(replacement_function);
  return option;
}


acr_option acr_new_alternative_parameter(const char* alternative_name,
                                         const char* parameter_to_swap,
                                         intmax_t replacement_value,
                                         size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->options.alternative.pragma_position = pragma_position;
  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_parameter;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  option->options.alternative.name_of_object_to_swap =
    acr_strdup(parameter_to_swap);
  option->options.alternative.swapped_by.replacement_value = replacement_value;
  return option;
}

acr_option acr_new_alternative_corner_computation(const char* alternative_name,
                                         size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->options.alternative.pragma_position = pragma_position;
  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_corner_computation;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  return option;
}

acr_option acr_new_alternative_zero_computation(const char* alternative_name,
                                         size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->options.alternative.pragma_position = pragma_position;
  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_zero_computation;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  return option;
}

acr_option acr_new_alternative_full_computation(const char* alternative_name,
                                         size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->options.alternative.pragma_position = pragma_position;
  option->type = acr_type_alternative;
  option->options.alternative.type = acr_alternative_full_computation;
  option->options.alternative.alternative_name = acr_strdup(alternative_name);
  return option;
}

acr_option acr_new_destroy(size_t pragma_position){
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_destroy;
  option->options.destroy.pragma_position = pragma_position;
  return option;
}

acr_option acr_new_grid(size_t grid_size,
    size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_grid;
  option->options.grid.grid_size = grid_size;
  option->options.grid.pragma_position = pragma_position;
  return option;
}

acr_option acr_new_init(const char* function_name,
                        size_t pragma_position,
                        size_t num_parameters,
                        acr_parameter_declaration* parameters_list) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_init;
  option->options.init.pragma_position = pragma_position;
  option->options.init.num_parameters = num_parameters;
  option->options.init.parameters_list = parameters_list;
  option->options.init.function_name = acr_strdup(function_name);
  return option;
}

acr_parameter_declaration* acr_new_parameter_declaration_list(
    size_t list_size) {
  if (list_size == 0)
    return NULL;
  acr_parameter_declaration* list = malloc(list_size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}


acr_parameter_specifier* acr_new_parameter_specifier_list(
    size_t list_size) {
  if (list_size == 0)
    return NULL;
  acr_parameter_specifier* list = malloc(list_size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}

acr_option acr_new_monitor(
    const struct acr_array_declaration* array_declaration,
    enum acr_monitor_processing_funtion processing_function,
    const char* filter_name,
    size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_monitor;
  option->options.monitor.data_monitored = *array_declaration;
  option->options.monitor.processing_function = processing_function;
  option->options.monitor.filter_name = acr_strdup(filter_name);
  option->options.monitor.pragma_position = pragma_position;
  return option;
}

acr_option acr_new_strategy_direct_int(const char* strategy_name,
                                       intmax_t matching_value,
                                       size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  option->options.strategy.strategy_type = acr_strategy_direct;
  option->options.strategy.value_type = acr_strategy_integer;
  option->options.strategy.boundaries[0].value_int = matching_value;
  option->options.strategy.boundaries[1].value_int = 0l;
  option->options.strategy.pragma_position = pragma_position;
  return option;
}

acr_option acr_new_strategy_range_int(const char* strategy_name,
                                      intmax_t matching_value[2],
                                      size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  if (matching_value[0] == matching_value[1])
    option->options.strategy.strategy_type = acr_strategy_direct;
  else
    option->options.strategy.strategy_type = acr_strategy_range;
  option->options.strategy.value_type = acr_strategy_integer;
  option->options.strategy.boundaries[0].value_int = matching_value[0];
  option->options.strategy.boundaries[1].value_int = matching_value[1];
  option->options.strategy.pragma_position = pragma_position;
  return option;
}

acr_option acr_new_strategy_direct_float(const char* strategy_name,
                                         float matching_value,
                                         size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  option->options.strategy.strategy_type = acr_strategy_direct;
  option->options.strategy.value_type = acr_strategy_floating_point;
  option->options.strategy.boundaries[0].value_float = matching_value;
  option->options.strategy.boundaries[1].value_float = 0.f;
  option->options.strategy.pragma_position = pragma_position;
  return option;
}

acr_option acr_new_strategy_range_float(const char* strategy_name,
                                        float matching_value[2],
                                        size_t pragma_position) {
  acr_option option = malloc(sizeof(*option));
  acr_try_or_die(option == NULL, "Malloc");

  option->type = acr_type_strategy;
  option->options.strategy.strategy_name = acr_strdup(strategy_name);
  if (float_equal(matching_value[0], matching_value[1]))
    option->options.strategy.strategy_type = acr_strategy_direct;
  else
    option->options.strategy.strategy_type = acr_strategy_range;
  option->options.strategy.value_type = acr_strategy_floating_point;
  option->options.strategy.boundaries[0].value_float = matching_value[0];
  option->options.strategy.boundaries[1].value_float = matching_value[1];
  option->options.strategy.pragma_position = pragma_position;
  return option;
}

acr_option_list acr_new_option_list(size_t size) {
  acr_option_list list = malloc(size * sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  return list;
}

static inline void acr_free_alternative(acr_alternative* alternative) {
  if (alternative->type == acr_alternative_function)
    free(alternative->swapped_by.replacement_function);
  free(alternative->alternative_name);
  if (alternative->type == acr_alternative_function || alternative->type == acr_alternative_parameter)
  free(alternative->name_of_object_to_swap);
}

static void acr_free_parameter_specifier_list(size_t num_specifiers,
    acr_parameter_specifier* parameter_specifiers_list) {
  if (!parameter_specifiers_list)
    return;
  for(size_t i = 0; i < num_specifiers; ++i) {
    free(parameter_specifiers_list[i].specifier);
  }
  free(parameter_specifiers_list);
}

static void acr_free_parameter_declaration_list(size_t num_parameters,
    acr_parameter_declaration* parameter_list) {
  if (!parameter_list)
    return;
  for(size_t i = 0; i < num_parameters; ++i) {
    free(parameter_list[i].parameter_name);
    acr_free_parameter_specifier_list(parameter_list[i].num_specifiers,
        parameter_list[i].parameter_specifiers_list);
  }
  free(parameter_list);
}

static inline void acr_free_init(acr_init* init) {
  free(init->function_name);
  acr_free_parameter_declaration_list(init->num_parameters,
      init->parameters_list);
}

void acr_free_acr_array_declaration(
    struct acr_array_declaration* array_declaration) {
  if (!array_declaration)
    return;
  acr_free_parameter_specifier_list(array_declaration->num_specifiers,
    array_declaration->parameter_specifiers_list);
  free(array_declaration->array_name);
  acr_free_array_dimensions_list(array_declaration->num_dimensions,
      array_declaration->array_dimensions_list);
}

static inline void acr_free_monitor(acr_monitor* monitor) {
  free(monitor->filter_name);
  acr_free_acr_array_declaration(&monitor->data_monitored);
}

static inline void acr_free_strategy(acr_strategy* strategy) {
  free(strategy->strategy_name);
}

static inline void acr_free_deferred_destroy(acr_deferred_destroy* def_dest) {
  free(def_dest->ref_init_name);
}

void acr_free_option(acr_option opt) {
  if (!opt)
    return;
  switch (opt->type) {
    case acr_type_alternative:
      acr_free_alternative(&opt->options.alternative);
      break;
    case acr_type_destroy:
    case acr_type_grid:
      break;
    case acr_type_init:
      acr_free_init(&opt->options.init);
      break;
    case acr_type_monitor:
      acr_free_monitor(&opt->options.monitor);
      break;
    case acr_type_strategy:
      acr_free_strategy(&opt->options.strategy);
      break;
    case acr_type_deferred_destroy:
      acr_free_deferred_destroy(&opt->options.deferred_destroy);
      break;
    case acr_type_unknown:
      break;
  }

  free(opt);
}

void acr_free_option_list(acr_option_list option_list, size_t size) {
  if (!option_list)
    return;
  for (size_t i = 0; i < size; ++i) {
    acr_free_option(acr_option_list_get_option(i, option_list));
  }
  free(option_list);
}

acr_compute_node acr_new_compute_node(size_t list_size,
    acr_option_list list) {
  acr_compute_node node = malloc(sizeof(*node));
  acr_try_or_die(node == NULL, "Malloc");
  node->option_list = list;
  node->list_size = list_size;
  return node;
}

void acr_free_compute_node(acr_compute_node node) {
  if (!node)
    return;
  acr_free_option_list(node->option_list, node->list_size);
  free(node);
}

acr_array_dimensions_list acr_new_array_dimensions_list(size_t size) {
  acr_array_dimensions_list dim = malloc(size * sizeof(*dim));
  acr_try_or_die(dim == NULL, "Malloc");
  return dim;
}

acr_compute_node_list acr_new_compute_node_list(size_t list_size) {
  if (list_size == 0)
    return NULL;
  acr_compute_node_list list = malloc(sizeof(*list));
  acr_try_or_die(list == NULL, "Malloc");
  list->list_size = list_size;
  if (list_size == 0)
    list->compute_node_list = NULL;
  else
    list->compute_node_list =
      malloc(list_size * sizeof(*list->compute_node_list));
  acr_try_or_die(list->compute_node_list == NULL, "Malloc");
  return list;
}

void acr_compute_node_delete_option_from_position(
    size_t position,
    acr_compute_node node) {
  acr_option_list option_list = acr_compute_node_get_option_list(node);
  size_t option_list_size = acr_compute_node_get_option_list_size(node);

  if (position < option_list_size) {
    acr_free_option(option_list[position]);
    for (size_t i = position; i + 1< option_list_size; ++i) {
      option_list[i] = option_list[i + 1];
    }
    option_list_size -= 1;
    node->option_list = realloc(node->option_list, option_list_size *
                                                   sizeof(*node->option_list));
    node->list_size = option_list_size;
  }
}

static bool acr_option_are_same_alternative(const acr_option alternative1,
                                            const acr_option alternative2) {
  if (acr_option_get_type(alternative1) == acr_type_alternative &&
      acr_option_get_type(alternative2) == acr_type_alternative) {
    char* name1 = alternative1->options.alternative.alternative_name;
    char* name2 = alternative2->options.alternative.alternative_name;
    return strcmp(name1, name2) == 0;
  }
  return false;
}

static bool acr_strategy_first_included_in_second(const acr_option strategy1,
                                                  const acr_option strategy2) {
  if (acr_option_get_type(strategy1) == acr_type_strategy &&
      acr_option_get_type(strategy2) == acr_type_strategy) {
    intmax_t val_int1[2];
    float val_float1[2];
    intmax_t val_int2[2];
    float val_float2[2];
    switch (acr_strategy_get_strategy_type(strategy1)) {
      case acr_strategy_direct:
        switch (acr_strategy_get_strategy_type(strategy2)) {
          case acr_strategy_direct:
            if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer &&
                acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
              acr_strategy_get_int_val(strategy1, val_int1);
              acr_strategy_get_int_val(strategy2, val_int2);
              return val_int1[0] == val_int2[0];
            } else {
              if (acr_strategy_get_value_type(strategy1) ==
                  acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_float_val(strategy2, val_float2);
                return float_equal(((float) val_int1[0]), val_float2[0]);
              } else {
                if (acr_strategy_get_value_type(strategy2) ==
                    acr_strategy_integer) {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_int_val(strategy2, val_int2);
                  return float_equal(val_float1[0], ((float) val_int2[0]));
                }
                else {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_float_val(strategy2, val_float2);
                  return float_equal(val_float1[0], val_float2[0]);
                }
              }
            }
            break;
          case acr_strategy_range:
            if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer &&
                acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_int_val(strategy2, val_int2);
                return val_int1[0] >= val_int2[0] && val_int1[0] <= val_int2[1];
            } else {
              if (acr_strategy_get_value_type(strategy1) ==
                  acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_float_val(strategy2, val_float2);
                return ((float) val_int1[0]) >= val_float2[0] &&
                       ((float) val_int1[0]) <= val_float2[1];
              } else {
                if (acr_strategy_get_value_type(strategy2) ==
                    acr_strategy_integer) {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_int_val(strategy2, val_int2);
                  return val_float1[0] >= ((float) val_int2[0]) &&
                         val_float1[0] <= ((float) val_int2[1]);
                }
                else {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_float_val(strategy2, val_float2);
                  return val_float1[0] >= val_float2[0] &&
                         val_float1[0] <= val_float2[1];
                }
              }
            }
            break;
          case acr_strategy_unknown:
            break;
        }
        break;
      case acr_strategy_range:
        switch (acr_strategy_get_strategy_type(strategy2)) {
          case acr_strategy_direct:
            return false;
            break;
          case acr_strategy_range:
            if (acr_strategy_get_value_type(strategy1) == acr_strategy_integer &&
                acr_strategy_get_value_type(strategy2) == acr_strategy_integer) {
              acr_strategy_get_int_val(strategy1, val_int1);
              acr_strategy_get_int_val(strategy2, val_int2);
              return val_int1[0] >= val_int2[0] && val_int1[1] <= val_int2[1];
            } else {
              if (acr_strategy_get_value_type(strategy1) ==
                  acr_strategy_integer) {
                acr_strategy_get_int_val(strategy1, val_int1);
                acr_strategy_get_float_val(strategy2, val_float2);
                return ((float) val_int1[0]) >= val_float2[0] &&
                       ((float) val_int1[1]) <= val_float2[1];
              } else {
                if (acr_strategy_get_value_type(strategy2) ==
                    acr_strategy_integer) {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_int_val(strategy2, val_int2);
                  return val_float1[0] >= ((float) val_int2[0]) &&
                        val_float1[1] <= ((float) val_int2[1]);
                } else {
                  acr_strategy_get_float_val(strategy1, val_float1);
                  acr_strategy_get_float_val(strategy2, val_float2);
                  return val_float1[0] >= val_float2[0] &&
                        val_float1[1] <= val_float2[1];
                }
              }
            }
            break;
          case acr_strategy_unknown:
            break;
        }
        break;
      case acr_strategy_unknown:
        break;
    }
  }
  return false;
}

bool acr_alternative_has_no_strategy_in_node(const acr_option alternative,
    const acr_compute_node node) {
  const char* alternative_name =
    acr_alternative_get_alternative_name(alternative);
  acr_option_list option_list = acr_compute_node_get_option_list(node);
  size_t list_size = acr_compute_node_get_option_list_size(node);
  for (size_t i = 0; i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, option_list);
    if (acr_option_get_type(current_option) == acr_type_strategy) {
      int diff = strcmp(acr_strategy_get_name(current_option), alternative_name);
      if (diff == 0)
        return false;
    }
  }
  fprintf(stderr,
      "[ACR] Warning: Alternative named %s has no known strategy using it.\n"
      "               Ignoring the following strategy:\n",
      alternative_name);
  pprint_acr_option(stderr, alternative, 0);
  return true;
}

bool acr_strategy_has_no_alternative_in_node(const acr_option strategy,
    const acr_compute_node node) {
  const char* strategy_name =
    acr_strategy_get_name(strategy);
  acr_option_list option_list = acr_compute_node_get_option_list(node);
  size_t list_size = acr_compute_node_get_option_list_size(node);
  for (size_t i = 0; i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, option_list);
    if (acr_option_get_type(current_option) == acr_type_alternative) {
      int diff = strcmp(acr_alternative_get_alternative_name(current_option),
          strategy_name);
      if (diff == 0)
        return false;
    }
  }
  fprintf(stderr,
      "[ACR] Warning: A strategy is using an undeclared alternative named \"%s\"\n"
      "               The following strategy will be ignored:\n",
      strategy_name);
  pprint_acr_option(stderr, strategy, 0);
  return true;
}

bool acr_simplify_compute_node(acr_compute_node node) {
  bool simplified = false;
  acr_option_list option_list;
  for (size_t i = 0; i < acr_compute_node_get_option_list_size(node); ++i) {
    option_list = acr_compute_node_get_option_list(node);
    switch (acr_option_get_type(acr_option_list_get_option(i, option_list))) {
      case acr_type_init:
        break;
      case acr_type_alternative:
        if(acr_alternative_has_no_strategy_in_node(
              acr_option_list_get_option(i, option_list), node)) {
          acr_compute_node_delete_option_from_position(i, node);
          i -= 1;
          simplified = true;
          break;
        }
        for (size_t j = 1; j < i; ++j) {
          option_list = acr_compute_node_get_option_list(node);
          acr_option to_compare = acr_option_list_get_option(j, option_list);
          if (acr_option_are_same_alternative(
                acr_option_list_get_option(i, option_list), to_compare)) {
            fprintf(stderr,
                "[ACR] Warning: Two alternatives with the same name,\n"
                "               The following alternative will be ignored:\n");
            pprint_acr_option(stderr, to_compare, 0);
            acr_compute_node_delete_option_from_position(j, node);
            i -= 1;
            j -= 1;
            simplified = true;
          }
        }
        break;
      case acr_type_grid:
      case acr_type_monitor:
      case acr_type_destroy:
        for (size_t j = 1; j < i; ++j) {
          option_list = acr_compute_node_get_option_list(node);
          acr_option to_compare = acr_option_list_get_option(j, option_list);
          if (acr_option_get_type(to_compare) ==
              acr_option_get_type(acr_option_list_get_option(i, option_list))) {
            acr_compute_node_delete_option_from_position(j, node);
            i -= 1;
            j -= 1;
            simplified = true;
          }
        }
        break;
      case acr_type_strategy:
        if(acr_strategy_has_no_alternative_in_node(
              acr_option_list_get_option(i, option_list), node)) {
          pprint_acr_option(stderr, acr_option_list_get_option(i, option_list), 0);
          acr_compute_node_delete_option_from_position(i, node);
          i -= 1;
          simplified = true;
          break;
        }
        for (size_t j = 1; j < i; ++j) {
            option_list = acr_compute_node_get_option_list(node);
            acr_option to_compare = acr_option_list_get_option(j, option_list);
          if (acr_strategy_first_included_in_second(to_compare,
                acr_option_list_get_option(i, option_list))) {
            fprintf(stderr,
                "[ACR] Warning: Two strategies overlap\n"
                "               The following strategy will be ignored:\n");
            pprint_acr_option(stderr, to_compare, 0);
            acr_compute_node_delete_option_from_position(j, node);
            i -= 1;
            j -= 1;
            simplified = true;
          } else {
            if (acr_strategy_first_included_in_second(
                  acr_option_list_get_option(i, option_list), to_compare)) {
              fprintf(stderr,
                  "[ACR] Warning: Two strategies overlap\n"
                  "               The following strategy will be ignored:\n");
              pprint_acr_option(stderr, acr_option_list_get_option(i, option_list), 0);
              acr_compute_node_delete_option_from_position(i, node);
              i -= 1;
              simplified = true;
              break;
            }
          }
        }
        break;
      case acr_type_deferred_destroy:
      case acr_type_unknown:
        break;
    }
  }
  return simplified;
}

static size_t acr_compute_node_count_num_type(
    const acr_compute_node node,
    enum acr_type type) {
  size_t num_type = 0;
  size_t list_size = acr_compute_node_get_option_list_size(node);
  const acr_option_list option_list = acr_compute_node_get_option_list(node);
  for (size_t i = 0; i < list_size; ++i) {
    if (acr_option_get_type(acr_option_list_get_option(i, option_list))
        == type) {
      num_type += 1;
    }
  }
  return num_type;
}

static inline size_t acr_option_list_position_of_next_type(
    const acr_option_list option_list,
    size_t list_size,
    size_t actual_position,
    enum acr_type type) {
  size_t i;
  for (i = actual_position; i < list_size; ++i) {
    if (acr_option_get_type(acr_option_list_get_option(i, option_list)) == type)
      return i;
  }
  return i;
}

acr_compute_node_list acr_new_compute_node_list_split_node(
    acr_compute_node node) {
  size_t num_init = acr_compute_node_count_num_type(node, acr_type_init);
  size_t num_dest =
    acr_compute_node_count_num_type(node, acr_type_destroy);
  if (num_init != num_dest || num_init == 0) {
    fprintf(stderr, "[ACR] Error: The number of init and destroy construct do"
        " not match or are equal to zero\n");
    acr_free_compute_node(node);
    return NULL;
  }
  acr_compute_node_list new_list = acr_new_compute_node_list(num_init);

  size_t list_size = acr_compute_node_get_option_list_size(node);
  const acr_option_list option_list = acr_compute_node_get_option_list(node);

  size_t next_destroy = 0;
  size_t actual_position = 0;
  for (size_t i = 0; i < num_init; ++i) {
    size_t next_init = acr_option_list_position_of_next_type(
        option_list, list_size, next_destroy, acr_type_init);
    next_destroy = acr_option_list_position_of_next_type(
        option_list, list_size, next_init, acr_type_destroy);
    if (next_init == list_size || next_destroy == list_size) {
      fprintf(stderr, "[ACR] Error: pragmas acr should be between only one"
          " init and one destroy\n");
      for (size_t j = 0; j < num_init; ++j) {
        if (j < i)
          acr_free_compute_node(acr_compute_node_list_get_node(j,new_list));
        acr_compute_node_list_set_node(j, NULL, new_list);
      }
      acr_free_compute_node_list(new_list);
      acr_free_compute_node(node);
      return NULL;
    }
    // Free bad positionned options
    for (size_t j = actual_position; j < next_init; ++j) {
      acr_free_option(acr_option_list_get_option(j, option_list));
      acr_option_list_set_option(NULL, j, option_list);
    }
    size_t new_size = next_destroy - next_init + 1;
    acr_option_list new_options = acr_new_option_list(new_size);
    for (size_t j = next_init; j <= next_destroy; ++j) {
      acr_option_list_set_option(acr_option_list_get_option(j, option_list),
          j - next_init, new_options);
      acr_option_list_set_option(NULL, j, option_list);
    }
    acr_compute_node new_node = acr_new_compute_node(new_size, new_options);
    while(acr_simplify_compute_node(new_node));
    acr_compute_node_list_set_node(i, new_node, new_list);
    next_destroy += 1;
    actual_position = next_destroy;
  }
  acr_free_compute_node(node);
  return new_list;
}

void acr_free_compute_node_list(acr_compute_node_list list) {
  if (!list)
    return;
  for (size_t i = 0; i < list->list_size; ++i) {
    acr_free_compute_node(list->compute_node_list[i]);
  }
  free(list->compute_node_list);
  free(list);
}

acr_option acr_copy_alternative(const acr_option alternative) {
  acr_alternative* alt = &alternative->options.alternative;
  switch (alternative->options.alternative.type) {
    case acr_alternative_parameter:
      return acr_new_alternative_parameter(alt->alternative_name,
          alt->name_of_object_to_swap, alt->swapped_by.replacement_value,
          alt->pragma_position);
      break;
    case acr_alternative_function:
      return acr_new_alternative_function(alt->alternative_name,
          alt->name_of_object_to_swap, alt->swapped_by.replacement_function,
          alt->pragma_position);
      break;
    case acr_alternative_corner_computation:
      return acr_new_alternative_corner_computation(alt->alternative_name,
          alt->pragma_position);
      break;
    case acr_alternative_zero_computation:
      return acr_new_alternative_zero_computation(alt->alternative_name,
          alt->pragma_position);
      break;
    case acr_alternative_full_computation:
      return acr_new_alternative_full_computation(alt->alternative_name,
          alt->pragma_position);
      break;
    case acr_alternative_unknown:
      break;
  }
  return NULL;
}

acr_option acr_copy_destroy(const acr_option destroy) {
  return acr_new_destroy(destroy->options.destroy.pragma_position);
}

acr_option acr_copy_grid(const acr_option grid) {
  return acr_new_grid(grid->options.grid.grid_size,
      grid->options.grid.pragma_position);
}

acr_parameter_specifier_list acr_copy_parameter_specifier_list(
    const acr_parameter_specifier_list specifier_list,
    size_t list_size) {

  acr_parameter_specifier_list new_list =
    acr_new_parameter_specifier_list(list_size);

  for (size_t nspec = 0; nspec < list_size; ++nspec) {
    acr_set_parameter_specifier(specifier_list[nspec].specifier,
        specifier_list[nspec].pointer_depth,
        &new_list[nspec]);
  }
  return new_list;
}

acr_parameter_declaration_list acr_copy_parameter_declaration_list(
    const acr_parameter_declaration_list parameter_list,
    size_t list_size) {

  acr_parameter_declaration_list new_parameter_list =
    acr_new_parameter_declaration_list(list_size);

  for (size_t nparam = 0; nparam < list_size; ++nparam) {
    acr_parameter_specifier_list new_specifier_list =
      acr_copy_parameter_specifier_list(
          parameter_list[nparam].parameter_specifiers_list,
          parameter_list[nparam].num_specifiers);
    acr_set_parameter_declaration(parameter_list[nparam].parameter_name,
        parameter_list[nparam].num_specifiers,
        new_specifier_list,
        &new_parameter_list[nparam]);
  }
  return new_parameter_list;
}

acr_array_dimension acr_copy_array_dimensions(acr_array_dimension dim) {
  acr_array_dimension copy = malloc(sizeof(*copy));
  copy->identifier = acr_strdup(dim->identifier);
  return copy;
}

void acr_copy_array_declaration(
    const acr_array_declaration* array_declaration,
    acr_array_declaration* new_array_dec) {
  new_array_dec->parameter_specifiers_list =
    acr_copy_parameter_specifier_list(
        array_declaration->parameter_specifiers_list,
        array_declaration->num_specifiers);
  new_array_dec->num_specifiers = array_declaration->num_specifiers;
  new_array_dec->array_name = acr_strdup(array_declaration->array_name);
  new_array_dec->array_dimensions_list =
    acr_new_array_dimensions_list(array_declaration->num_dimensions);
  new_array_dec->num_dimensions = array_declaration->num_dimensions;
  for (size_t i = 0 ; i< new_array_dec->num_dimensions; ++i) {
    new_array_dec->array_dimensions_list[i] = acr_copy_array_dimensions(array_declaration->array_dimensions_list[i]);
  }
}

acr_option acr_copy_init(const acr_option init) {
  acr_init* ini = &init->options.init;
  acr_parameter_declaration_list new_list =
    acr_copy_parameter_declaration_list(ini->parameters_list,
        ini->num_parameters);
  return acr_new_init(ini->function_name, ini->pragma_position,
      ini->num_parameters, new_list);
}

acr_option acr_copy_monitor(const acr_option monitor) {
  acr_monitor* mon = &monitor->options.monitor;
  acr_array_declaration declaration_copy;
  acr_copy_array_declaration(&monitor->options.monitor.data_monitored,
      &declaration_copy);
  return acr_new_monitor(&declaration_copy, mon->processing_function,
      mon->filter_name, mon->pragma_position);
}

acr_option acr_copy_strategy(const acr_option strategy) {
  acr_option new_strategy = malloc(sizeof(*new_strategy));
  *new_strategy = *strategy;
  new_strategy->options.strategy.strategy_name =
    acr_strdup(new_strategy->options.strategy.strategy_name);
  return new_strategy;
}

acr_option acr_copy_deferred_destroy(const acr_option def_dest) {
  return acr_new_deferred_destroy(
      acr_deferred_destroy_get_pragma_position(def_dest),
      acr_deferred_destroy_get_ref_init_name(def_dest));
}

acr_option acr_copy_option(const acr_option option) {
  switch (option->type) {
    case acr_type_alternative:
      return acr_copy_alternative(option);
    case acr_type_destroy:
      return acr_copy_destroy(option);
    case acr_type_grid:
      return acr_copy_grid(option);
    case acr_type_init:
      return acr_copy_init(option);
    case acr_type_monitor:
      return acr_copy_monitor(option);
    case acr_type_strategy:
      return acr_copy_strategy(option);
    case acr_type_deferred_destroy:
      return acr_copy_deferred_destroy(option);
    case acr_type_unknown:
      return NULL;
  }
  return NULL;
}

acr_compute_node acr_copy_compute_node(const acr_compute_node node) {
  acr_compute_node newnode = malloc(sizeof(*newnode));
  newnode->list_size = node->list_size;
  newnode->option_list = acr_new_option_list(node->list_size);
  for (size_t i = 0; i < node->list_size; ++i) {
    newnode->option_list[i] = acr_copy_option(node->option_list[i]);
  }
  return newnode;
}

acr_compute_node_list acr_copy_compute_node_list(
    const acr_compute_node_list list) {
  acr_compute_node_list newlist =
    acr_new_compute_node_list(list->list_size);
  for (size_t i = 0; i < list->list_size; ++i) {
    newlist->compute_node_list[i] =
      acr_copy_compute_node(list->compute_node_list[i]);
  }
  return newlist;
}

size_t acr_option_get_pragma_position(const acr_option option) {
  switch (acr_option_get_type(option)) {
    case acr_type_alternative:
      return acr_alternative_get_pragma_position(option);
      break;
    case acr_type_destroy:
      return acr_destroy_get_pragma_position(option);
      break;
    case acr_type_grid:
      return acr_grid_get_pragma_position(option);
      break;
    case acr_type_init:
      return acr_init_get_pragma_position(option);
      break;
    case acr_type_monitor:
      return acr_monitor_get_pragma_position(option);
      break;
    case acr_type_strategy:
      return acr_strategy_get_pragma_position(option);
      break;
    case acr_type_deferred_destroy:
      return acr_deferred_destroy_get_pragma_position(option);
    case acr_type_unknown:
      break;
  }
  return 0;
}

acr_option acr_compute_node_get_option_of_type(enum acr_type search_type,
    const acr_compute_node node, size_t which_one) {
  acr_option_list list = acr_compute_node_get_option_list(node);
  size_t list_size = acr_compute_node_get_option_list_size(node);

  size_t how_many_found = 0;
  for (size_t i = 0; i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, list);
    if (acr_option_get_type(current_option) == search_type) {
      how_many_found += 1;
      if (how_many_found == which_one)
        return current_option;
    }
  }
  return NULL;
}

acr_array_dimension acr_new_array_dimensions(const char* identifier) {
  acr_array_dimension new_dim = malloc(sizeof(*new_dim));
  new_dim->identifier = acr_strdup(identifier);
  return new_dim;
}

bool acr_strategy_correspond_to_alternative(
    const acr_option strategy,
    const acr_option alternative) {
  const char *strategy_name = acr_strategy_get_name(strategy);
  const char *alternative_name =
    acr_alternative_get_alternative_name(alternative);
  return strcmp(strategy_name, alternative_name) == 0;
}

acr_option acr_new_deferred_destroy(size_t pragma_position,
    const char *ref_init_name) {
  acr_option option = malloc(sizeof(*option));
  option->type = acr_type_deferred_destroy;
  option->options.deferred_destroy.pragma_position = pragma_position;
  option->options.deferred_destroy.ref_init_name = acr_strdup(ref_init_name);
  return option;
}

acr_option_list acr_get_general_option_list(const acr_compute_node node,
    size_t *new_list_size) {
  *new_list_size = 0;
  const acr_option_list option_list = acr_compute_node_get_option_list(node);
  size_t list_size = acr_compute_node_get_option_list_size(node);
  for (size_t i = 0; i < list_size; ++i) {
    const acr_option opt = acr_option_list_get_option(i, option_list);
    switch (acr_option_get_type(opt)) {
      case acr_type_deferred_destroy:
        *new_list_size += 1;
        break;
      case acr_type_alternative:
      case acr_type_destroy:
      case acr_type_grid:
      case acr_type_init:
      case acr_type_monitor:
      case acr_type_strategy:
      case acr_type_unknown:
        break;
    }
  }
  if (*new_list_size == 0)
    return NULL;
  acr_option_list general_options = acr_new_option_list(*new_list_size);
  size_t option_number = 0;
  for (size_t i = 0; i < list_size; ++i) {
    const acr_option opt = acr_option_list_get_option(i, option_list);
    switch (acr_option_get_type(opt)) {
      case acr_type_deferred_destroy:
        acr_option_list_set_option(acr_copy_option(opt), option_number++,
            general_options);
          break;
      case acr_type_alternative:
      case acr_type_destroy:
      case acr_type_grid:
      case acr_type_init:
      case acr_type_monitor:
      case acr_type_strategy:
      case acr_type_unknown:
        break;
    }
  }
  return general_options;
}
