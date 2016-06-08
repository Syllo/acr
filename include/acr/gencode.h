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
 * \file gencode.h
 * \brief Generate the ACR instrumented code
 *
 * \defgroup build_gencode
 *
 * @{
 * \brief Generate the instrumented code from ACR pragma construct
 *
 */

#ifndef __ACR_GENCODE_H
#define __ACR_GENCODE_H

#include <stdio.h>

#include "acr/acr_openscop.h"
#include "acr/acr_options.h"
#include "acr/pragma_struct.h"

/**
 * \brief Given a filename and the options, generate the instrumented code that
 * will contain the acr runtime.
 * \param[in] filename The filename to parse.
 * \param[in] build_options The build options.
 *
 * The new file will be generated in the same directory as the given file and
 * will end up with the same name whith "-acr" append at the end.
 */
void acr_generate_code(const char* filename,
    struct acr_build_options *build_options);

/**
 * \brief Print only the acr pragma and kernel information and exit.
 * \param[in,out] out The output stream.
 * \param[in] filename The filename to parse.
 */
void acr_print_structure_and_related_scop(FILE* out, const char* filename);

/**
 * \brief Generate the preamble
 * \param[in,out] out The output stream.
 * \param[in] filename The filename to parse.
 *
 * The preamble contains information about the initial file and a pretty good
 * joke.
 *
 */
void acr_generate_preamble(FILE* out, const char *filename);

/**
 * \brief Print the acr structure related data.
 * \param[in,out] kernel_file The input file stream
 * \param[in,out] out The output stream
 * \param[in] node The compute node
 * \param[in] kernel_start The number of characters from the start of the file
 * where to find the kernel
 * \param[in] kernel_end The number of characters from the start of the file
 * where to find the end of the kernel
 * \retval true If there is something to initialize
 * \retval false Otherwise
 * \sa build_pragma
 * \sa start_acr_parsing
 */
bool acr_print_node_initialization(FILE* kernel_file, FILE* out,
    const acr_compute_node node,
    size_t kernel_start, size_t kernel_end);

/**
 * \brief Extract the OpenScop format of the kernel inside the compute node.
 * \param[in] node The compute node.
 * \param[in,out] input_file The input file where to search the kernel.
 * \param[in] name_input_file The name of the input file
 * \return The OpenScop format of the kernel corresponding to the node
 * \sa build_pragma
 * \sa start_acr_parsing
 */
osl_scop_p acr_extract_scop_in_compute_node(const acr_compute_node node,
                                            FILE* input_file,
                                            const char* name_input_file);

/**
 * \brief Extract two bounds where the compute kernel should be found
 * \param[in] node The compute node
 * \param[out] start The starting point where to search the kernel
 * \param[out] stop The ending point where to search the kernel
 * \sa build_pragma
 */
void acr_get_start_and_stop_for_clan(const acr_compute_node node,
    size_t *start, size_t *stop);

/**
 * \brief Translate OpenScop coordinates to acr coordinates
 * \param[in,out] kernel_file The stream where to find the Scop
 * \param[in] scop The OpenScop format
 * \param[in] compute_node The compute node
 * \param[out] start The starting point of the kernel
 * \param[out] end The ending point of the kernel
 * \sa build_pragma
 */
void acr_scop_coord_to_acr_coord(
    FILE *kernel_file,
    const osl_scop_p scop,
    const acr_compute_node compute_node,
    size_t *start,
    size_t *end);

/**
 * \brief Print the acr data initialization function that must be called at
 * runtime
 * \param[in,out] out The output stream
 * \param[in] node The compute node
 * \sa build_pragma
 */
void acr_print_node_init_function_call(FILE* out,
    const acr_compute_node node);

/**
 * \brief Print the acr data initialization function that must be called at
 * runtime for a performance run test
 * \param[in,out] out The output stream
 * \param[in] node The compute node
 * \sa build_pragma
 */
void acr_print_node_init_function_call_for_max_perf_run(FILE *out,
    const acr_compute_node node);

#endif // __ACR_GENCODE_H

/**
 *
 * @}
 *
 */
