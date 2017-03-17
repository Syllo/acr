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

/**
 *
 * \file pragma_struct.h
 * \brief ACR pragma construct functions
 *
 * \defgroup build_pragma
 *
 * @{
 * \brief ACR pragma construct build/destroy/explore functions
 *
 * \defgroup build_print Print the pragma data structure
 *
 */

#ifndef __ACR_PRAGMA_STRUCT_H
#define __ACR_PRAGMA_STRUCT_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "acr/utils.h"

/**
 * \brief The alternative construct type
 */
enum acr_alternative_type {
  /** \brief Parameter modification alternative */
  acr_alternative_parameter = 0,
  /** \brief Function swap alternative */
  acr_alternative_function,
  /** \brief Only compute in corners */
  acr_alternative_corner_computation,
  /** \brief Not compute at all */
  acr_alternative_zero_computation,
  /** \brief Do it all */
  acr_alternative_full_computation,
  /** \brief Number of alternative types */
  acr_alternative_unknown,
};

/**
 * \brief The alternative data structure
 */
typedef struct acr_alternative {
  /** \brief The position of the pragma in the file */
  size_t pragma_position;
  /** \brief The alternative data */
  union {
  /** \brief Parameter value */
    intmax_t replacement_value;
  /** \brief Alternative function name to call */
    char* replacement_function;
  } swapped_by;
  /** \brief Name of the parameter or function bound to the alternative */
  char* name_of_object_to_swap;
  /** \brief The alternative name */
  char* alternative_name;
  /** \brief The alternative type */
  enum acr_alternative_type type;
} acr_alternative;

/**
 * \brief The destroy data structure
 */
typedef struct acr_destroy {
  /** \brief Position of the pragma in the file */
  size_t pragma_position;
} acr_destroy;

/**
 * \brief The grid data structure
 */
typedef struct acr_grid {
  /** \brief Position of the pragma in the file */
  size_t pragma_position;
  /** \brief The tiling size */
  size_t grid_size;
} acr_grid;

/**
 * \brief The parameter specifier data structure
 */
typedef struct acr_parameter_specifier {
  /** \brief The number of pointer sign */
  size_t pointer_depth;
  /** \brief the parameter specifier */
  char* specifier;
} acr_parameter_specifier,
  /** \brief A parameter specifier list */
  *acr_parameter_specifier_list;

/**
 * \brief The parameter declaration data structure
 */
typedef struct acr_parameter_declaration {
  /** \brief Number of specifiers */
  size_t num_specifiers;
  /** \brief The specifiers list */
  acr_parameter_specifier_list parameter_specifiers_list;
  /** \brief The parameter identifier */
  char* parameter_name;
} acr_parameter_declaration,
  /** \brief Parameter declaration list */
  *acr_parameter_declaration_list;

/**
 * \brief The init data structure
 */
typedef struct acr_init {
  /** \brief Position of the pragma in the file */
  size_t pragma_position;
  /** \brief The kernel function name */
  char* function_name;
  /** \brief Number of parameters that the function takes */
  size_t num_parameters;
  /** \brief The parameters list */
  acr_parameter_declaration_list parameters_list;
} acr_init;

/**
 * \brief The array dimension data structure
 */
typedef struct acr_array_dimension {
  /** \brief The dimension value */
  char *identifier;
} *acr_array_dimension,
  /** \brief The array dimension list */
  **acr_array_dimensions_list;

/**
 * \brief The array declaration data structure
 */
typedef struct acr_array_declaration {
  /** \brief The number of array specifiers */
  size_t num_specifiers;
  /** \brief The specifiers list */
  acr_parameter_specifier_list parameter_specifiers_list;
  /** \brief The array name */
  char* array_name;
  /** \brief The number of dimensions of the array */
  size_t num_dimensions;
  /** \brief The dimensions of the array */
  acr_array_dimensions_list array_dimensions_list;
} acr_array_declaration;

/**
 * \brief The processing function type
 */
enum acr_monitor_processing_funtion {
  /** \brief The min function */
  acr_monitor_function_min = 0,
  /** \brief The max function */
  acr_monitor_function_max,
  /** \brief The average function */
  acr_monitor_function_avg,
  /** \brief The number of functions */
  acr_monitor_function_unknown,
};

/**
 * \brief The monitor data structure
 */
typedef struct acr_monitor {
  /** \brief Position of the pragma in the file */
  size_t pragma_position;
  /** \brief The filter function name */
  char* filter_name;
  /** \brief The array to monitor */
  acr_array_declaration data_monitored;
  /** \brief The processing function */
  enum acr_monitor_processing_funtion processing_function;
} acr_monitor;

/**
 * \brief The strategy type
 */
enum acr_strategy_type {
  /** \brief Strategy direct */
  acr_strategy_direct = 0,
  /** \brief Strategy between bounds */
  acr_strategy_range,
  /** \brief Number of strategies */
  acr_strategy_unknown,
};

/**
 * \brief Strategie value type
 */
enum acr_strategy_value_type {
  /** \brief An integer type */
  acr_strategy_integer = 0,
  /** \brief A floating point type */
  acr_strategy_floating_point,
};

/**
 * \brief The strategy data structure
 */
typedef struct acr_strategy {
  /** \brief Position of the pragma in the file */
  size_t pragma_position;
  /** \brief The strategy type */
  enum acr_strategy_type strategy_type;
  /** \brief The value type */
  enum acr_strategy_value_type value_type;
  /** \brief The value */
  union {
  /** \brief The value as an integer */
    intmax_t value_int;
  /** \brief The value as a floating point */
    float value_float;
  } boundaries[2];
  /** \brief The strategy name */
  char* strategy_name;
} acr_strategy;

/**
 * \brief The deferred destroy data structure
 */
typedef struct acr_deferred_destroy {
  /** \brief Position of the pragma in the file */
  size_t pragma_position;
  /** \brief The init function name */
  char *ref_init_name;
} acr_deferred_destroy;

/**
 * \brief The acr structures type
 */
enum acr_type {
  /** \brief An alternative */
  acr_type_alternative = 0,
  /** \brief A destroy */
  acr_type_destroy,
  /** \brief A grid */
  acr_type_grid,
  /** \brief An init */
  acr_type_init,
  /** \brief A monitor */
  acr_type_monitor,
  /** \brief A strategy */
  acr_type_strategy,
  /** \brief A deferred destroy */
  acr_type_deferred_destroy,
  /** \brief The number of structure types */
  acr_type_unknown,
};

/**
 * \brief The option data structure
 */
typedef struct acr_option {
  /** \brief The option data */
  union {
  /** \brief Alternative */
    acr_alternative alternative;
  /** \brief Destroy */
    acr_destroy destroy;
  /** \brief Deferred destroy */
    acr_deferred_destroy deferred_destroy;
  /** \brief Grid */
    acr_grid grid;
  /** \brief Init */
    acr_init init;
  /** \brief Monitor */
    acr_monitor monitor;
  /** \brief Strategy */
    acr_strategy strategy;
  } options;
  /** \brief Option type */
  enum acr_type type;
} *acr_option,
  /** \brief The option list */
  **acr_option_list;

/**
 * \brief Compute node data structure
 */
typedef struct acr_compute_node {
  /** \brief Number of options in the list */
  size_t list_size;
  /** \brief The option list */
  acr_option_list option_list;
} *acr_compute_node;

/**
 * \brief Compute node list data structure
 */
typedef struct acr_compute_node_list {
  /** \brief The number of compute node in the list */
  size_t list_size;
  /** \brief The compute node list */
  acr_compute_node* compute_node_list;
} *acr_compute_node_list;

/**
 * \brief Get the type of the option
 * \param[in] option The acr option
 * \return The type of the option
 */
static inline enum acr_type acr_option_get_type(acr_option option) {
  return option->type;
}

/**
 * \brief Create a new alternative function
 * \param[in] alternative_name The tag of the alternative
 * \param[in] function_to_swap The function to swap identifier
 * \param[in] replacement_function The replacement function identifier
 * \param[in] pragma_position The position in file
 * \return An alternative option
 */
acr_option acr_new_alternative_function(const char* alternative_name,
                                        const char* function_to_swap,
                                        const char* replacement_function,
                                        size_t pragma_position);

/**
 * \brief Create a new alternative parameter
 * \param[in] alternative_name The tag of the alternative
 * \param[in] parameter_to_swap The identifier of the parameter to swap
 * \param[in] replacement_value The replacement value
 * \param[in] pragma_position The position in file
 * \return An alternative option
 */
acr_option acr_new_alternative_parameter(const char* alternative_name,
                                         const char* parameter_to_swap,
                                         intmax_t replacement_value,
                                         size_t pragma_position);

/**
 * \brief Create a new alternative corner computation
 * \param[in] alternative_name The tag of the alternative
 * \param[in] pragma_position The position in file
 * \return An alternative option
 */
acr_option acr_new_alternative_corner_computation(const char* alternative_name,
                                         size_t pragma_position);

/**
 * \brief Create a new alternative zero computation
 * \param[in] alternative_name The tag of the alternative
 * \param[in] pragma_position The position in file
 * \return An alternative option
 */
acr_option acr_new_alternative_zero_computation(const char* alternative_name,
                                         size_t pragma_position);

/**
 * \brief Create a new alternative full computation
 * \param[in] alternative_name The tag of the alternative
 * \param[in] pragma_position The position in file
 * \return An alternative option
 */
acr_option acr_new_alternative_full_computation(const char* alternative_name,
                                         size_t pragma_position);

/**
 * \brief Copy an alternative
 * \param[in] alternative The alternative to copy
 * \return The copy
 */
acr_option acr_copy_alternative(const acr_option alternative);

/**
 * \brief Get the alternative type
 * \param[in] option The alternative
 * \return The type of the alternative
 */
static inline enum acr_alternative_type acr_alternative_get_type(
    const acr_option option) {
  return option->options.alternative.type;
}

/**
 * \brief Get the alternative name
 * \param[in] option The alternative
 * \return The tag of the alternative
 */
static inline char* acr_alternative_get_alternative_name(
    const acr_option option) {
  return option->options.alternative.alternative_name;
}

/**
 * \brief Get the alternative identifier of the object to swap
 * \param[in] option The alternative
 * \return The identifier of the object to swap
 */
static inline char* acr_alternative_get_object_to_swap_name(
    const acr_option option){
  return option->options.alternative.name_of_object_to_swap;
}

/**
 * \brief Get the replacement function of an alternative function
 * \param[in] option The alternative
 * \return The replacement function identifier
 */
static inline char* acr_alternative_get_replacement_function(
    const acr_option option){
  return acr_alternative_get_type(option) == acr_alternative_function ?
    option->options.alternative.swapped_by.replacement_function :
    NULL;
}

/**
 * \brief Get the replacement parameter of an alternative parameter
 * \param[in] option The alternative
 * \return The parameter identifier
 */
static inline intmax_t acr_alternative_get_replacement_parameter(
    const acr_option option){
  return acr_alternative_get_type(option) == acr_alternative_parameter ?
    option->options.alternative.swapped_by.replacement_value :
    0;
}

/**
 * \brief Get the pragma position
 * \param[in] option The alternative
 * \return The pragma position in the file
 */
static inline size_t acr_alternative_get_pragma_position(
    const acr_option option) {
  return option->options.alternative.pragma_position;
}

/**
 * \brief Create a new destroy option
 * \param[in] pragma_position The pragma position in file
 * \return A new destroy option
 */
acr_option acr_new_destroy(size_t pragma_position);

/**
 * \brief Copy a destroy option
 * \param[in] destroy The destroy option
 * \return A copy of the destroy option
 */
acr_option acr_copy_destroy(const acr_option destroy);

/**
 * \brief Get the destroy pragma position
 * \param[in] option The destroy option
 * \return The destroy pragma position in file
 */
static inline size_t acr_destroy_get_pragma_position(
    const acr_option option) {
  return option->options.destroy.pragma_position;
}

/**
 * \brief Create a new grid option
 * \param[in] grid_size The tiling size
 * \param[in] pragma_position The pragma position in file
 * \return A new grid option
 */
acr_option acr_new_grid(size_t grid_size,
                        size_t pragma_position);

/**
 * \brief Copy a grid option
 * \param[in] grid The grid option
 * \return A copy of the grid option
 */
acr_option acr_copy_grid(const acr_option grid);

/**
 * \brief Get the grid size
 * \param[in] option The grid option
 * \return The grid size
 */
static inline size_t acr_grid_get_grid_size(
    const acr_option option) {
  return option->options.grid.grid_size;
}

/**
 * \brief Get the grid pragma position
 * \param[in] option The grid option
 * \return The grid pragma position in file
 */
static inline size_t acr_grid_get_pragma_position(const acr_option option) {
  return option->options.grid.pragma_position;
}

/**
 * \brief Create a new init option
 * \param[in] function_name The tag of the init function
 * \param[in] pragma_position The init pragma position in file
 * \param[in] num_parameters The number of parameters of the function
 * \param[in] parameters_list The parameter list
 * \return The new init option
 */
acr_option acr_new_init(const char* function_name,
                        size_t pragma_position,
                        size_t num_parameters,
                        acr_parameter_declaration_list parameters_list);

/**
 * \brief Copy an init option
 * \param[in] init The init option
 * \return A copy of the init option
 */
acr_option acr_copy_init(const acr_option init);

/**
 * \brief Get the init function name
 * \param[in] option The init option
 * \return The init function name
 */
static inline char* acr_init_get_function_name(const acr_option option) {
  return option->options.init.function_name;
}

/**
 * \brief Get the init pragma position
 * \param[in] option The init option
 * \return The init pragma position in file
 */
static inline size_t acr_init_get_pragma_position(
    const acr_option option) {
  return option->options.init.pragma_position;
}

/**
 * \brief Get the init option function parameter number
 * \param[in] option The init option
 * \return The init option function parameter number
 */
static inline size_t acr_init_get_num_parameters(
    const acr_option option) {
  return option->options.init.num_parameters;
}

/**
 * \brief Get the init option parameter list
 * \param[in] option The init option
 * \return The init option parameter list
 */
static inline acr_parameter_declaration_list acr_init_get_parameter_list(
    const acr_option option) {
  return option->options.init.parameters_list;
}

/**
 * \brief Create a new parameter declaration list
 * \param[in] list_size The size of the list to allocate
 * \return The list having list_size elements
 */
acr_parameter_declaration_list acr_new_parameter_declaration_list(
    size_t list_size);

/**
 * \brief Copy a parameter declaration list
 * \param[in] parameter_list The parameters list
 * \param[in] list_size The size of the list
 * \return A copy of the parameter declaration list
 */
acr_parameter_declaration_list acr_copy_parameter_declaration_list(
    const acr_parameter_declaration_list parameter_list,
    size_t list_size);

/**
 * \brief Set a parameter declaration value
 * \param[in] parameter_name The parameter identifier
 * \param[in] num_specifiers The number of parameter specifiers
 * \param[in] specifier_list The specifier list
 * \param[out] declaration_to_set The declaration to set
 */
static inline void acr_set_parameter_declaration(
    const char* parameter_name,
    size_t num_specifiers,
    acr_parameter_specifier_list specifier_list,
    acr_parameter_declaration_list declaration_to_set) {
  declaration_to_set->parameter_specifiers_list = specifier_list;
  declaration_to_set->num_specifiers = num_specifiers;
  declaration_to_set->parameter_name = acr_strdup(parameter_name);
}

/**
 * \brief Get the parameter identifier
 * \param[in] declaration_list A declaration list
 * \param[in] position_in_list Which declaration identifier to choose from
 * \return The parameter identifier
 */
static inline char* acr_parameter_declaration_get_parameter_name(
    const acr_parameter_declaration_list declaration_list,
    size_t position_in_list) {
  return declaration_list[position_in_list].parameter_name;
}

/**
 * \brief Get the number of specifiers of a declaration
 * \param[in] declaration_list A declaration list
 * \param[in] position_in_list The position in the list
 * \return The number of specifiers if this declaration
 */
static inline size_t acr_parameter_declaration_get_num_specifiers(
    const acr_parameter_declaration_list declaration_list,
    size_t position_in_list) {
  return declaration_list[position_in_list].num_specifiers;
}

/**
 * \brief Get the declaration specifier list
 * \param[in] declaration_list A declaration list
 * \param[in] position_in_list The position in the list
 * \return The declaration specifier list
 */
static inline acr_parameter_specifier_list acr_parameter_declaration_get_specif_list(
    const acr_parameter_declaration_list declaration_list,
    size_t position_in_list) {
  return declaration_list[position_in_list].parameter_specifiers_list;
}

/**
 * \brief Create a new parameter specifier list
 * \param[in] list_size The size of the list to allocate
 * \return The new allocated list
 */
acr_parameter_specifier_list acr_new_parameter_specifier_list(
    size_t list_size);

/**
 * \brief Set a parameter specifier in the list
 * \param[in] specifier_name The specifier name
 * \param[in] pointer_depth The pointer depth
 * \param[out] specifier The specifier list
 */
static inline void acr_set_parameter_specifier(
    const char* specifier_name,
    size_t pointer_depth,
    acr_parameter_specifier_list specifier) {
  specifier->specifier = acr_strdup(specifier_name);
  specifier->pointer_depth = pointer_depth;
}

/**
 * \brief Test if the specifier is a pointer
 * \param[in] specifier The parameter specifier list
 * \param[in] position_in_list The position in the list
 * \retval true If it is a pointer
 * \return false Othrewise
 */
static inline bool acr_parameter_specifier_is_pointer(
    const acr_parameter_specifier_list specifier,
    size_t position_in_list) {
  return specifier[position_in_list].pointer_depth > 0;
}

/**
 * \brief Get the pointer depth
 * \param[in] specifier The parameter specifier list
 * \param[in] position_in_list The position in the list
 * \return The pointer depth
 */
static inline size_t acr_parameter_specifier_get_pointer_depth(
    const acr_parameter_specifier_list specifier,
    size_t position_in_list) {
  return specifier[position_in_list].pointer_depth;
}

/**
 * \brief Get the specifier value
 * \param[in] specifier The parameter specifier list
 * \param[in] position_in_list The position in the list
 * \return The specifier value
 */
static inline char* acr_parameter_specifier_get_specifier(
    const acr_parameter_specifier_list specifier,
    size_t position_in_list) {
  return specifier[position_in_list].specifier;
}

/**
 * \brief Create a new monitor option
 * \param[in] array_declaration An array declaration
 * \param[in] processing_function The processing function
 * \param[in] filter_name The filter name
 * \param[in] pragma_position The pragma position in file
 * \return A new monitor option
 */
acr_option acr_new_monitor(
    const acr_array_declaration* array_declaration,
    enum acr_monitor_processing_funtion processing_function,
    const char* filter_name,
    size_t pragma_position);

/**
 * \brief Copy a monitor option
 * \param[in] monitor The monitor option
 * \return A copy of the monitor option
 */
acr_option acr_copy_monitor(const acr_option monitor);

/**
 * \brief Get the pragma position
 * \param[in] option The monitor option
 * \return The monitor pragma position in file
 */
static inline size_t acr_monitor_get_pragma_position(const acr_option option) {
  return option->options.monitor.pragma_position;
}

/**
 * \brief Get the array declaration
 * \param[in] option The monitor option
 * \return The array declaration
 */
static inline acr_array_declaration* acr_monitor_get_array_declaration(
    acr_option option) {
  return &option->options.monitor.data_monitored;
}

/**
 * \brief Get the processing function
 * \param[in] option The monitor option
 * \return The processing function
 */
static inline enum acr_monitor_processing_funtion acr_monitor_get_function(
    acr_option option) {
  return option->options.monitor.processing_function;
}

/**
 * \brief Get the filter name
 * \param[in] option The monitor option
 * \return The filter name
 */
static inline char* acr_monitor_get_filter_name(acr_option option) {
  return option->options.monitor.filter_name;
}

/**
 * \brief Set a new array declaration
 * \param[in] num_specifiers The number of specifiers
 * \param[in] parameters_list The parameters list
 * \param[in] array_name The array name
 * \param[in] num_dimensions The number of dimensions
 * \param[in] array_dimensions The array dimensions value
 * \param[out] array_declaration The array declaration to set
 */
static inline void acr_set_array_declaration(
    size_t num_specifiers,
    acr_parameter_specifier_list parameters_list,
    const char* array_name,
    size_t num_dimensions,
    acr_array_dimensions_list array_dimensions,
    struct acr_array_declaration* array_declaration) {
  array_declaration->array_dimensions_list = array_dimensions;
  array_declaration->num_dimensions = num_dimensions;
  array_declaration->num_specifiers = num_specifiers;
  array_declaration->array_name = acr_strdup(array_name);
  array_declaration->parameter_specifiers_list = parameters_list;
}

/**
 * \brief Get the number of specifiers
 * \param[in] declaration The array declaration
 * \return The number of specifiers
 */
static inline size_t acr_array_decl_get_num_specifiers(
    acr_array_declaration* declaration) {
  return declaration->num_specifiers;
}

/**
 * \brief Get the specifiers list
 * \param[in] declaration The array declaration
 * \return The specifiers list
 */
static inline acr_parameter_specifier_list acr_array_decl_get_specifiers_list(
    acr_array_declaration* declaration) {
  return declaration->parameter_specifiers_list;
}

/**
 * \brief Get the array name
 * \param[in] declaration The array declaration
 * \return The array name
 */
static inline char* acr_array_decl_get_array_name(
    acr_array_declaration* declaration) {
  return declaration->array_name;
}

/**
 * \brief Get the number of dimensions of the array
 * \param[in] declaration The array declaration
 * \return The number of dimensions of the array
 */
static inline size_t acr_array_decl_get_num_dimensions(
    acr_array_declaration* declaration) {
  return declaration->num_dimensions;
}

/**
 * \brief Get the dimensions value list of the array
 * \param[in] declaration The array declaration
 * \return The dimensions list of the array
 */
static inline acr_array_dimensions_list acr_array_decl_get_dimensions_list(
    acr_array_declaration* declaration) {
  return declaration->array_dimensions_list;
}

/**
 * \brief Create a new strategy direct integer
 * \param[in] strategy_name The strategy tag
 * \param[in] matching_value The strategy value
 * \param[in] pragma_position The pragma position in file
 * \return A new strategy direct integer
 */
acr_option acr_new_strategy_direct_int(const char* strategy_name,
                                       intmax_t matching_value,
                                       size_t pragma_position);

/**
 * \brief Create a new strategy direct float
 * \param[in] strategy_name The strategy tag
 * \param[in] matching_value The strategy value
 * \param[in] pragma_position The pragma position in file
 * \return A new strategy direct float
 */
acr_option acr_new_strategy_direct_float(const char* strategy_name,
                                         float matching_value,
                                         size_t pragma_position);

/**
 * \brief Create a new strategy range integer
 * \param[in] strategy_name The strategy tag
 * \param[in] matching_value The strategy values
 * \param[in] pragma_position The pragma position in file
 * \return A new strategy range integer
 * \brief
 */
acr_option acr_new_strategy_range_int(const char* strategy_name,
                                      intmax_t matching_value[2],
                                      size_t pragma_position);

/**
 * \brief Create a new strategy range float
 * \param[in] strategy_name The strategy tag
 * \param[in] matching_value The strategy values
 * \param[in] pragma_position The pragma position in file
 * \return A new strategy range float
 */
acr_option acr_new_strategy_range_float(const char* strategy_name,
                                        float matching_value[2],
                                        size_t pragma_position);

/**
 * \brief Copy a strategy option
 * \param[in] strategy The strategy option
 * \return A copy of the strategy
 */
acr_option acr_copy_strategy(const acr_option strategy);

/**
 * \brief Get the pragma position in file
 * \param[in] option The Strategy option
 * \return The strategy pragma position in file
 */
static inline size_t acr_strategy_get_pragma_position(const acr_option option) {
  return option->options.strategy.pragma_position;
}

/**
 * \brief Get the strategy type
 * \param[in] option The Strategy option
 * \return The strategy type
 */
static inline enum acr_strategy_type acr_strategy_get_strategy_type(
    const acr_option option) {
  return option->options.strategy.strategy_type;
}

/**
 * \brief Get the strategy tag
 * \param[in] option The Strategy option
 * \return The strategy tag
 */
static inline char* acr_strategy_get_name(const acr_option option) {
  return option->options.strategy.strategy_name;
}

/**
 * \brief Get the strategy value type
 * \param[in] option The Strategy option
 * \return The strategy value type
 */
static inline enum acr_strategy_value_type acr_strategy_get_value_type(
    const acr_option option) {
  return option->options.strategy.value_type;
}

/**
 * \brief Get the strategy float values
 * \param[in] option The Strategy option
 * \param[out] values The values of the strategy
 */
static inline void acr_strategy_get_float_val(
    const acr_option option,
    float values[2]){
  values[0] = option->options.strategy.boundaries[0].value_float;
  values[1] = option->options.strategy.boundaries[1].value_float;
}

/**
 * \brief Get the strategy integer values
 * \param[in] option The Strategy option
 * \param[out] values The values of the strategy
 */
static inline void acr_strategy_get_int_val(
    const acr_option option,
    intmax_t values[2]){
  values[0] = option->options.strategy.boundaries[0].value_int;
  values[1] = option->options.strategy.boundaries[1].value_int;
}

/**
 * \brief Create a new option list
 * \param[in] size The list size to allocate
 * \return A new option list
 */
acr_option_list acr_new_option_list(size_t size);

/**
 * \brief Set an option in the option list
 * \param[in] option The option to put in the list
 * \param[in] position The position in the list
 * \param[in,out] list The option list
 */
static inline void acr_option_list_set_option(acr_option option,
                                              size_t position,
                                              acr_option_list list) {
  list[position] = option;
}

/**
 * \brief Get an option in the option list
 * \param[in] position The position in the list
 * \param[in,out] list The option list
 * \return The option at the indicated position
 */
static inline acr_option acr_option_list_get_option(size_t position,
                                                    acr_option_list list) {
  return list[position];
}

/**
 * \brief Free an acr option
 * \param[in] option An acr option
 */
void acr_free_option(acr_option option);

/**
 * \brief Free an option list and its content
 * \param[in] option_list The option list
 * \param[in] size The list size
 */
void acr_free_option_list(acr_option_list option_list, size_t size);

/**
 * \brief Create a new compute node
 * \param[in] list_size The size of the list inside the node
 * \param[in] list The list itself
 * \return The compute node
 */
acr_compute_node acr_new_compute_node(size_t list_size,
    acr_option_list list);

/**
 * \brief Free a compute node and its content
 * \param[in] node The compute node to free
 */
void acr_free_compute_node(acr_compute_node node);

/**
 * \brief Get the option list
 * \param[in] node The compute node
 * \return The option list
 */
static inline acr_option_list acr_compute_node_get_option_list(
    acr_compute_node node) {
  return node->option_list;
}

/**
 * \brief Get the option list size
 * \param[in] node The compute node
 * \return The option list size
 */
static inline size_t acr_compute_node_get_option_list_size(
    acr_compute_node node) {
  return node->list_size;
}

/**
 * \brief Free an array_declaration
 * \param[in] array_declaration The array declaration
 */
void acr_free_acr_array_declaration(
    struct acr_array_declaration* array_declaration);

/**
 * \brief Create a new array dimensions list
 * \param[in] size The size of the list to allocate
 * \return The new array dimension list
 */
acr_array_dimensions_list acr_new_array_dimensions_list(size_t size);

/**
 * \brief Free an array dimension
 * \param[in] dim An array dimension
 */
static inline void acr_free_array_dimension(acr_array_dimension dim) {
  if (!dim)
    return;
  free(dim->identifier);
  free(dim);
}

/**
 * \brief Create a new array dimension
 * \param[in] identifier The dimension value
 * \return The new array dimension
 */
acr_array_dimension acr_new_array_dimensions(const char* identifier);

/**
 * \brief Free an array dimension list
 * \param[in] list_size The size of the array dimensions list
 * \param[in] list The array dimensions list
 */
static inline void acr_free_array_dimensions_list(
    size_t list_size,
    acr_array_dimensions_list list) {
  if (!list)
    return;
  for (size_t i = 0; i < list_size; ++i) {
    acr_free_array_dimension(list[i]);
  }
  free(list);
}

/**
 * \brief Copy an array dimension
 * \param[in] dim The array dimension
 * \return The array dimension copy
 */
acr_array_dimension acr_copy_array_dimensions(acr_array_dimension dim);

/**
 * \brief Get the dimension value
 * \param[in] dim The array dimension
 * \return The dimension value
 */
static inline char* acr_array_dimension_get_identifier(acr_array_dimension dim){
  return dim->identifier;
}

/**
 * \brief Create a new compute node list
 * \param[in] list_size The size of the list to allocate
 * \return The new node list
 */
acr_compute_node_list acr_new_compute_node_list(size_t list_size);

/**
 * \brief Free a compute node list
 * \param[in] list The node list
 */
void acr_free_compute_node_list(acr_compute_node_list list);

/**
 * \brief Set a node in the node list
 * \param[in] position The position in the list
 * \param[in] node The node to add to the list
 * \param[in,out] node_list The node list
 */
static inline void acr_compute_node_list_set_node(
    size_t position,
    const acr_compute_node node,
    acr_compute_node_list node_list) {
  node_list->compute_node_list[position] = node;
}

/**
 * \brief Get a node in the list
 * \param[in] position The position in the list
 * \param[in] node_list The node list
 * \return The node at the given position
 */
static inline acr_compute_node acr_compute_node_list_get_node(
    size_t position,
    const acr_compute_node_list node_list) {
  return node_list->compute_node_list[position];
}

/**
 * \brief Get the size of the node list
 * \param[in] node_list The node list
 * \return The size of the node list
 */
static inline size_t acr_compute_node_list_get_size(
    const acr_compute_node_list node_list) {
  return node_list->list_size;
}

/**
 * \brief Split the initial merged node into multiple compute node
 * \param[in,out] node The node to split
 * \sa start_acr_parsing
 * \return A node list with each option in a node related to the same kernel
 */
acr_compute_node_list acr_new_compute_node_list_split_node(
    acr_compute_node node);

/**
 * \brief Copy an option
 * \param[in] option The option to copy
 * \return A copy of the option
 */
acr_option acr_copy_option(const acr_option option);

/**
 * \brief Copy a compute node
 * \param[in] node The node to copy
 * \return A copy of the node
 */
acr_compute_node acr_copy_compute_node(const acr_compute_node node);

/**
 * \brief Copy a compute node list
 * \param[in] list The compute node list to copy
 * \return The copy of the node list
 */
acr_compute_node_list acr_copy_compute_node_list(
    const acr_compute_node_list list);

/**
 * \brief Copy a parameter specifier list
 * \param[in] specifier_list The specifier list
 * \param[in] list_size The size of the list
 * \return The copy of the specifier list
 */
acr_parameter_specifier_list acr_copy_parameter_specifier_list(
    const acr_parameter_specifier_list specifier_list,
    size_t list_size);

/**
 * \brief Get the option pragma position
 * \param[in] option The acr option
 * \return The pragma position of the option in file
 */
size_t acr_option_get_pragma_position(const acr_option option);

/**
 * \brief Copy an array declaration
 * \param[in] array_declaration The array declaration
 * \param[out] new_array_dec The copy of the array declaration
 */
void acr_copy_array_declaration(
    const acr_array_declaration* array_declaration,
    acr_array_declaration* new_array_dec);

/**
 * \brief Get an option of a given type in a node
 * \param[in] option_type The type of the option to seek
 * \param[in] node The node where to search for the option
 * \param[in] which_one The option index of the search (for example 2 will give
 * the second encountered option of the given type)
 * \retval NULL If not found or the index is too big
 * \return The option corresponding to the given type and index
 */
acr_option acr_compute_node_get_option_of_type(enum acr_type option_type,
    const acr_compute_node node, size_t which_one);

/**
 * \brief Test if the alternative match a strategy in the node
 * \param[in] alternative The alternative
 * \param[in] node The node where to search
 * \retval true A strategy has been fount
 * \retval false Otherwise
 */
bool acr_alternative_has_no_strategy_in_node(const acr_option alternative,
    const acr_compute_node node);

/**
 * \brief Search if a strategy match an alternative in the node
 * \param[in] strategy The strategy
 * \param[in] node The node where to search
 * \retval true An alternative has been fount
 * \retval false Otherwise
 */
bool acr_strategy_has_no_alternative_in_node(const acr_option strategy,
    const acr_compute_node node);

/**
 * \brief Test if a strategy match a given alternative
 * \param[in] strategy The strategy
 * \param[in] alternative The alternative
 * \retval true The alternative matches the strategy
 * \retval false Otherwise
 */
bool acr_strategy_correspond_to_alternative(
    const acr_option strategy,
    const acr_option alternative);

/**
 * \brief Delete an option from a position in the node
 * \param[in] position The position where to delete
 * \param[in,out] node The compute node
 */
void acr_compute_node_delete_option_from_position(
    size_t position,
    acr_compute_node node);

/**
 * \brief Delete unused and duplicate acr options in node
 * \param[in,out] node The node to simplify
 * \retval true The node has been simplified
 * \retval false Otherwise
 */
bool acr_simplify_compute_node(acr_compute_node node);

/**
 * \brief Create a new defered destroy option
 * \param[in] pragma_position The pragma position in file
 * \param[in] ref_init_name The init name it matches
 * \return A new deferred destroy option
 */
acr_option acr_new_deferred_destroy(size_t pragma_position,
    const char *ref_init_name);

/**
 * \brief Get matching init name
 * \param[in] option A deferred destroy option
 * \return The init matching name
 */
static inline char* acr_deferred_destroy_get_ref_init_name(acr_option option) {
  assert(acr_option_get_type(option) == acr_type_deferred_destroy);
  return option->options.deferred_destroy.ref_init_name;
}

/**
 * \brief Get the pragma position
 * \param[in] option A deferred destroy option
 * \return The pragma position in file
 */
static inline size_t acr_deferred_destroy_get_pragma_position(acr_option option) {
  assert(acr_option_get_type(option) == acr_type_deferred_destroy);
  return option->options.deferred_destroy.pragma_position;
}

/**
 * \brief Copy a deferred destroy option
 * \param[in] def_dest The deferred destroy option
 * \return The copy of the deferred destroy option
 */
acr_option acr_copy_deferred_destroy(const acr_option def_dest);

/**
 * \brief Extract file scope acr option from the list
 * \param[in] node The node having all the options
 * \param[out] new_list_size The size of the newly created list
 * \return The list with the file scope options
 * \sa start_acr_parsing
 */
acr_option_list acr_get_general_option_list(const acr_compute_node node,
    size_t *new_list_size);

#endif // __ACR_PRAGMA_STRUCT_H

/**
 *
 * @}
 *
 */
