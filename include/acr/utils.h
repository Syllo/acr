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
 * \file utils.h
 * \brief Utilities functions
 *
 * \defgroup acr_misc Miscellaneous
 *
 * @{
 * \brief General purpose functions and data
 *
 * \defgroup acr_utilities Utilities functions
 * \defgroup acr_time Time related functions
 * \defgroup acr_compiler_path The compiler absolute path
 *
 * @}
 *
 * \defgroup acr_utilities
 *
 * @{
 * \brief Useful functions and macros
 *
 */

#ifndef __ACR_UTILS_H
#define __ACR_UTILS_H

#include <stdio.h>

#define acr_try_or_die(A,B) {if((A)){perror(B); exit(EXIT_FAILURE);}}

#ifdef ACR_DEBUG
  #define acr_print_debug(A,B)  { fprintf(A, "%s\n", B); }
#else
  #define acr_print_debug(A,B)  {  }
#endif // ACR_DEBUG

char* acr_strdup(const char* to_duplicate);

#define FLOAT_TRESHOLD 1e-5f
#define float_equal(a,b) ((((a)-(b)) <= FLOAT_TRESHOLD) && \
                          (((a)-(b)) >= (-FLOAT_TRESHOLD)))

#define DOUBLE_TRESHOLD 1e-5
#define double_equal(a,b) ((((a)-(b)) <= DOUBLE_TRESHOLD) && \
                          (((a)-(b)) >= (-DOUBLE_TRESHOLD)))

#endif // __ACR_UTILS_H

/**
 *
 * @}
 *
 */
