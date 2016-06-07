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
 * \file acr_openscop.h
 * \brief Extracr information from OpenScop format
 *
 * \defgroup build_openscop
 *
 * @{
 * \brief Extract information from OpenScop format
 *
 */

#ifndef __ACR_OPENSCOP_H
#define __ACR_OPENSCOP_H

#include "acr/pragma_struct.h"

#include <osl/scop.h>
#include <isl/set.h>

/**
 * \brief Enumeration giving the type of a dimension related to acr
 */
enum acr_dimension_type {
  /** \brief An iterator that has an alternative construct dependency */
  acr_dimension_type_bound_to_alternative,
  /** \brief An iterator that has an monitor construct dependency */
  acr_dimension_type_bound_to_monitor,
  /** \brief An iterator that has no acr related dependencies */
  acr_dimension_type_free_dim,
};

/**
 * \brief Upper and lower bound information for every dimensions in a loop
 */
typedef struct dimensions_upper_lower_bounds {
  /** \brief Total number of dimensions of the loop nest */
  size_t num_dimensions;
  /** \brief Total number of input parameters of the loop nest */
  size_t num_parameters;
  /** \brief Dependencies of lower bound between dimensions
   *
   * For example for \f$x>0\f$,
   * \f$|lower\_bound[x]| = x-1\f$ and if \f$lower\_bound[x][0]\f$ is true, the
   * \f$x_{th}\f$ lower bound depends on the dimension 0 value
   */
  bool **lower_bound;
  /** \brief See dimensions_upper_lower_bounds::lower_bound */
  bool **upper_bound;
  /** \brief The lexicographical minimum for each bounds */
  isl_set *bound_lexmin;
  /** \brief The lexicographical maximum for each bounds */
  isl_set *bound_lexmax;
  /** \brief The dimension type */
  enum acr_dimension_type *dimensions_type;
  /** \brief True if a dimension has a dependency from a previous one
   *
   * For \f$x > 0, |has\_constraint\_with\_previous\_dim[x]| = x-1\f$ and if
   * \f$has\_constraint\_with\_previous\_dim[x][0]\f$ is true, the \f$x_{th}\f$
   * dimension has a constraint with the \f$0_{th}\f$ dimension
   */
  bool **has_constraint_with_previous_dim;
} dimensions_upper_lower_bounds;

/**
 * \brief For each statements, his upper and lower bound informations
 */
typedef struct dimensions_upper_lower_bounds_all_statements {
  /** \brief The total number of statements */
  size_t num_statements;
  /** \brief An array of size num_statements storing pointers */
  dimensions_upper_lower_bounds **statements_bounds;
} dimensions_upper_lower_bounds_all_statements;

/**
 * \brief Dump the OpenScop format inside a memory buffer
 * \param[in] scop The scop to dump
 * \param[in,out] buffer A pointer to a char*. The buffer will be automatically
 * allocated and must be freed once you are done with it. The size of the buffer
 * will be **AT LEAST** size_buffer.
 * \param[in,out] size_buffer A pointer to a size_t that will be initialized
 * with the number of characters written in the buffer.
 */
void acr_print_scop_to_buffer(const osl_scop_p scop, char** buffer,
    size_t* size_buffer);

/**
 * \brief Generate a kernel that will scan the monitored dataset
 * \param[in] monitor The monitor acr option
 * \param[in] scop The scop The OpenScop representation of the loop
 * \param[in] grid_size The tiling size used for the data
 * \param[in] dims The bounds for all statements
 * \param[in,out] bound_used Return the bounds used to scan the monitoring domain
 * \param[in] prefix The acr kernel name prefix
 * \return An OpenScop representation of the loop that will scan the monitored
 * data and store the data to an unsigned char array.
 * \sa start_acr_parsing
 * \sa build_pragma
 *
 * Generate a function of prototype ``void (*)(unsigned char *result)`` that
 * will scan each tile and store the result inside of the result array.
 *
 */
osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const osl_scop_p scop,
    size_t grid_size,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used,
    const char *prefix);

/**
 * \brief Extract the dimensions name that scans the data.
 * \param monitor The monitor acr option
 * \return An OpenScop string that contains a null terminated string list.
 * \sa build_pragma
 */
osl_strings_p acr_openscop_get_monitor_parameters(const acr_option monitor);

/**
 * \brief Swap two columns in an OpenScop relation
 * \param[in,out] relation The OpenScop relation
 * \param[in] pos1 The first index
 * \param[in] pos2 The second index
 */
void osl_relation_swap_column(osl_relation_p relation,
    int pos1,
    int pos2);

/**
 * \brief Generate the index array of the monitor dim
 * \param[in] dims The upper and lower bound informations
 * \param[in] num_dims The number of dimensions
 * \param[in,out] monitor_dim An array initialized with the monitor bound
 * dimensions index
 * \sa acr_osl_get_min_max_bound_statement
 * \sa acr_osl_get_upper_lower_bound_all
 */
void monitor_openscop_generate_monitor_dim(
    const dimensions_upper_lower_bounds *dims,
    size_t num_dims,
    size_t **monitor_dim);

/**
 * \brief Modify the scop to use min or max aggregation function
 * \param[in] monitor The monitor acr option
 * \param[in] filter_function The name of the function to call to convert each
 * monitored data to an unsigned char. NULL if none.
 * \param[in] grid_size The tiling size for the data
 * \param[in] max True to use the **max** function, false to use **min**.
 * \param[in] monitor_dim An array filled with the index of the monitor
 * dimension
 * \param[in,out] scop The OpenScop format to modify
 * \param[in] prefix The prefix used for the kernel
 * \sa build_pragma
 * \sa monitor_openscop_generate_monitor_dim
 */
void acr_openscop_set_tiled_to_do_min_max(
    const acr_option monitor,
    const char* filter_function,
    size_t grid_size,
    bool max,
    const size_t *monitor_dim,
    osl_scop_p scop,
    const char *prefix);

/**
 * \brief Modify the scop to use the average aggregation function
 * \param[in] monitor The monitor acr option
 * \param[in] filter_function The name of the function to call to convert each
 * monitored data to an unsigned char. NULL if none.
 * \param[in] grid_size The tiling size for the data
 * \param[in] monitor_dim An array filled with the index of the monitor
 * dimension
 * \param[in,out] scop The OpenScop format to modify
 * \param[in] prefix The prefix used for the kernel
 * \sa build_pragma
 * \sa monitor_openscop_generate_monitor_dim
 */
void acr_openscop_set_tiled_to_do_avg(
    const acr_option monitor,
    const char* filter_function,
    size_t grid_size,
    const size_t *monitor_dim,
    osl_scop_p scop,
    const char *prefix);

/**
 * \brief Extract the data about the bounds from all the statements in the
 * OpenScop statement list
 * \param[in] statement_list The OpenScop statement list
 * \return The data bounds data from all the statements
 * \remark You should call ::acr_osl_find_and_verify_free_dims_position after
 * this function to initialize the dimensions type.
 */
dimensions_upper_lower_bounds_all_statements* acr_osl_get_upper_lower_bound_all(
        const osl_statement_p statement_list);

/**
 * \brief Destructor of the ::dimensions_upper_lower_bounds data structure
 * \param[in] bounds The data to free
 */
void acr_osl_free_dimension_upper_lower_bounds(
    dimensions_upper_lower_bounds *bounds);

/**
 * \brief Destructor for the ::dimensions_upper_lower_bounds_all_statements
 * data structure
 * \param[in] bounds_all The data to free
 */
void acr_osl_free_dimension_upper_lower_bounds_all(
    dimensions_upper_lower_bounds_all_statements *bounds_all);

/**
 * \brief Initialize the type of each dimension and check if the acr semantic is
 * respected
 * \param[in] node The acr node
 * \param[in] scop The OpenScop representation of the loop
 * \param[in,out] bounds_all The bounds extracted from the OpenScop format
 * \retval true If the node and scope respect the acr semantic
 * \retval false Otherwise
 * \sa acr_osl_get_upper_lower_bound_all
 * \sa start_acr_parsing
 */
bool acr_osl_find_and_verify_free_dims_position(
    const acr_compute_node node,
    const osl_scop_p scop,
    dimensions_upper_lower_bounds_all_statements *bounds_all);

/**
 * \brief Test if a dimension has an constraint with another one
 * \param[in] dim1 The first dimension
 * \param[in] dim2 The second dimension
 * \param[in] bounds The upper and lower bound informations
 * \pre \f$dim1 > dim2\f$
 * \retval true If dim1 has a constraint with dim2
 * \retval false Otherwise
 */
bool acr_osl_dim_has_constraints_with_dim(
    size_t dim1,
    size_t dim2,
    const dimensions_upper_lower_bounds *bounds);

/**
 * \brief Test if a given dimension has any dependencies coming from a previous
 * one
 * \param[in] dim1 The dimension
 * \param[in] bounds The upper and lower bound informations
 * \retval true If dim1 has at least one constraint with a previous dimension
 * \retval false Otherwise
 */
bool acr_osl_dim_has_constraints_with_previous_dims(
    size_t dim1,
    const dimensions_upper_lower_bounds *bounds);

/**
 * \brief Convert the monitor dimension identifiers to the OpenScop string
 * format.
 * \param[in] monitor The monitor acr option
 * \return A NULL terminated string array filled with the monitor dimension
 * names
 */
osl_strings_p acr_openscop_get_monitor_identifiers(const acr_option monitor);

/**
 * \brief Check if any of the identifiers are not loop parameters.
 * \param[in] scop The OpenScop format data
 * \param[in] identifiers Some identifiers names
 * \retval true If none of the identifiers is a parameter
 * \retval false Otherwise
 */
bool acr_osl_check_if_identifiers_are_not_parameters(const osl_scop_p scop,
    const osl_strings_p identifiers);

/**
 * \brief Get all the monitor bound dimension names and their dependencies
 * dimension names
 * \param[in] monitor The monitor acr option
 * \param[in] scop The OpenScop format data
 * \param[in] dims The upper and lower bound informations
 * \param[in,out] bound_used The bound where all the monitoring dimensions are
 * present at once
 * \param[in,out] all_iterators The names of all the monitor bound dimensions
 * and their dependencies dimension names ordered by dimension level.
 * \param[in,out] statement_containing_them The related statement where they
 * have been extracted
 */
void acr_openscop_get_identifiers_with_dependencies(
    const acr_option monitor,
    const osl_scop_p scop,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used,
    osl_strings_p *all_iterators,
    osl_statement_p *statement_containing_them);

/**
 * \brief Get the bounds informations out of a statement
 * \param[in] statement An OpenScop representation of a statement
 * \return The bounds informations.
 */
dimensions_upper_lower_bounds* acr_osl_get_min_max_bound_statement(
    const osl_statement_p statement);

/**
 * \brief Get the names of all the parameters used in acr alternative constructs
 * \param[in] node The acr node
 * \return An OpenScop NULL terminated string with the parameters names
 * \sa start_acr_parsing
 */
osl_strings_p acr_osl_get_alternative_parameters(
    const acr_compute_node node);

#endif // __ACR_OPENSCOP_H

/**
 *
 * @}
 *
 */
