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
 * \file acr_options.h
 * \brief ACR compiler options
 *
 * \defgroup build_options
 *
 * @{
 * \brief Compiler options
 *
 */

#ifndef __ACR_OPTIONS_H
#define __ACR_OPTIONS_H

enum acr_build_type {
  acr_regular_build,
  acr_perf_kernel_only,
  acr_perf_compile_time_zero,
  acr_perf_compile_time_zero_run,
};

struct acr_build_options {
  enum acr_build_type type;
};

#endif // __ACR_OPTIONS_H

/**
 *
 * @}
 *
 */
