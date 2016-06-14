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
 * \file runtime_alternatives.h
 * \brief Structure to store alternative data
 *
 * \defgroup runtime_alternative
 *
 * @{
 * \brief Structure storing the alternative data required during runtime
 *
 */

#ifndef __ACR_RUNTIME_ALTERNATIVES__H
#define __ACR_RUNTIME_ALTERNATIVES__H

#include <stdint.h>
#include <isl/set.h>

/**
 * \brief The runtime alternative type
 */
enum acr_runtime_alternative_type {
  /** \brief It is an alternative modifying a parameter */
  acr_runtime_alternative_parameter,
  /** \brief It is an alternative calling a function */
  acr_runtime_alternative_function,
};

/**
 * \brief Structure storing the alternative info needed at runtime
 */
struct runtime_alternative {
  /** \brief The initial iteration domains for each statements for each threads */
  isl_set ***restricted_domains;
  /** \brief The number of alternatives for this kernel */
  size_t alternative_number;
  /** \brief The alternative information */
  struct {
    /** \brief Union for function and parameter data compaction */
    union {
      /** \brief The replacement function for function type */
      char *function_to_swap;
      /** \brief The data for parameter type */
      struct {
        /** \brief The function computing the whole kernel with this alternative */
        void *function_matching_alternative;
        /** \brief The parameter value */
        intmax_t parameter_value;
        /** \brief The parameter position */
        unsigned int parameter_id;
      } parameter;
    } alt;
    /** \brief Name of parameter or function that the alternative modifies */
    char *name_to_swap;
  } value;
  /** \brief The alternative type */
  enum acr_runtime_alternative_type type;
};

#endif // __ACR_RUNTIME_ALTERNATIVES__H

/**
 *
 * @}
 *
 */
