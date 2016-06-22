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
 * \file acr_runtime_code_generation.h
 * \brief Code generation using CLooG library
 *
 * \defgroup runtime_generation
 *
 * @{
 * \brief Runtime code generation using the polyhedral model and CLooG
 *
 */

#ifndef __ACR_RUNTIME_CODE_GENERATION_H
#define __ACR_RUNTIME_CODE_GENERATION_H

#include <acr/acr_runtime_data.h>
#include <acr/runtime_alternatives.h>
#include <isl/set.h>

/**
 * \brief Generate an optimized code based on the current data observation using
 * CLooG.
 * \param[in,out] output The output file where to store the generated code.
 * \param[in] data_info The runtime data info
 * \param[in] data The array representation of the alternative state to use.
 * \param[in] thread_num The id of the thread that requested the generation.
 * \param[in] generation_buffer A buffer of size the number of alternatives
 * \sa runtime_data
 */
void acr_cloog_generate_alternative_code_from_input(
    FILE* output,
    const struct acr_runtime_data *data_info,
    const unsigned char *data,
    size_t thread_num,
    isl_set **generation_buffer);

/**
 * \brief Update the OpenScop used for code generation with current alternative.
 * \param[in,out] data The runtime data info
 * \sa runtime_data
 */
void acr_gencode_init_scop_to_match_alternatives(
    struct acr_runtime_data *data);

/**
 * \brief At runtime we know the parameters so this function replaces the
 * parameters of the loop with their respective values.
 * \param[in,out] data_info The runtime data info
 * \param[in] parameter_index The parameter index
 * \param[in] value The current parameter value
 * \sa runtime_data
 */
void acr_cloog_get_rid_of_parameter(
    struct acr_runtime_data *data_info,
    unsigned int parameter_index,
    long int value);

/**
 * \brief At runtime we know the parameters so this function replaces the
 * parameters of the loop with their respective values.
 * \param[in,out] data_info The runtime data info
 * \param[in] parameter_index The parameter index
 * \param[in] value The current parameter value
 * \sa runtime_data
 */
void acr_cloog_get_rid_of_parameter_static(
    struct acr_runtime_data_static *data_info,
    unsigned int parameter_index,
    long int value);

#endif // __ACR_RUNTIME_CODE_GENERATION_H

/**
 *
 * @}
 *
 */
