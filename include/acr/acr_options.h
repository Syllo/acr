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

/**
 * \brief The ACR strategy to choose
 *
 * ACR can use multiple strategies and that can be used when generating
 * the code.
 */
enum acr_build_type {
  /** \brief Build a runnable version of ACR */
  acr_regular_build,
  /** \brief Build a version that will output the sequential code to run only
   * the kernel
   */
  acr_perf_kernel_only,
  /** \brief Build a version that will generate all the functions in the working
   * directory.
   */
  acr_perf_compile_time_zero,
  /** \brief Build a version that will run the generated functions instead of
   * ACR runtime thus providing an approximation of ACR without compile time.
   */
  acr_perf_compile_time_zero_run,
};

/**
 * \brief The runtime kernel type to use
 */
enum acr_runtime_kernel_version {
  /** \brief ACR simple strategy
   *
   * This Strategy just monitors the data and issue a compilation if the
   * currently used version is no more suitable
   */
  acr_runtime_kernel_simple,
  /** \brief ACR versioning strategy
   *
   * This strategy uses versions of the kernel to minimize the need
   * recompilation in case of many local fluctuations.
   */
  acr_runtime_kernel_versioning,
  /** \brief ACR stencil strategy
   *
   * This kernel uses a stencil approach to anticipate the change due to
   * neighborhood changing state.
   */
  acr_runtime_kernel_stencil,
  /** \brief Error value
   */
  acr_runtime_kernel_unknown,
};

/**
 * \brief Struct storing the build type during code generation
 */
struct acr_build_options {
  /** \brief The build type */
  enum acr_build_type type;
  /** \brief The kernel version used */
  enum acr_runtime_kernel_version kernel_version;
};

#endif // __ACR_OPTIONS_H

/**
 *
 * @}
 *
 */
