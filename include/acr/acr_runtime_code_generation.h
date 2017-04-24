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

/**
 * \brief Take a domain to tile and generate a function for each tiles.
 * \param[in] static_data The static data structure storing the kernel infos
 * \param[out] num_tiles The number of tiles functions generated
 * \param[out] tiles_functions The string with the tiled functions
 */
void acr_code_generation_generate_tiling_library(
    struct acr_runtime_data_static *static_data,
    char **tiles_functions);

/**
 * \brief Take a domain union and apply a tiling on. IT starts at a given
 * dimension and do a tiling for the given nth underneath
 * \param[in] tiling_size The size of each tile
 * \param[in] dimension_start_tiling The first dimension to start tiling
 * \param[in] num_dimensions_to_tile The number of dimension to tile
 * \param[in,out] ud The union domain to tile
 */
void acr_runtime_apply_tiling(
    size_t tiling_size,
    size_t dimension_start_tiling,
    size_t num_dimensions_to_tile,
    CloogUnionDomain *ud);

void acr_runtime_apply_reduction_function(
    CloogDomain *context,
    CloogUnionDomain const *ud,
    size_t first_monitor_dimension,
    size_t number_of_monitoring_dimensions,
    CloogUnionDomain **new_ud,
    CloogDomain **new_context,
    osl_scop_p *new_scop,
    const char *const scan_corpse,
    char const*const iterators[],
    bool keep_parameters);

void acr_generation_generate_tiling_alternatives(
    size_t grid_size,
    size_t first_monitor_dimension,
    size_t num_monitor_dimensions,
    osl_scop_p scop,
    acr_compute_node node);

#endif // __ACR_RUNTIME_CODE_GENERATION_H

/**
 *
 * @}
 *
 */
