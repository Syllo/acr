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

#include "acr/acr.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include "acr/gencode.h"

static const char opt_options[] = "abpvVhxyz";

int main(int argc, char** argv) {

  struct acr_build_options build_options =
  {
    .type = acr_regular_build,
    .kernel_version = acr_runtime_kernel_simple,
  };

  bool print = false;

  for (;;) {
    int c = getopt(argc, argv, opt_options);
    if (c == -1) // No more candies
      break;

    switch (c) {
      case 'a':
        build_options.kernel_version = acr_runtime_kernel_versioning;
        break;
      case 'b':
        build_options.kernel_version = acr_runtime_kernel_stencil;
        break;
      case 'x':
        build_options.type = acr_perf_compile_time_zero_run;
        break;
      case 'z':
        build_options.type = acr_perf_kernel_only;
        break;
      case 'y':
        build_options.type = acr_perf_compile_time_zero;
        break;
      case 'h':
        fprintf(stdout, help);
          return EXIT_SUCCESS;
        break;
      case 'v':
      case 'V':
        fprintf(stdout, "%s", version);
          return EXIT_SUCCESS;
        break;
      case 'p':
        print = true;
        break;
      default:
        fprintf(stderr, "Unknown option: %c\n", c);
        break;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc > 0) {
    for (int i = 0; i < argc; ++i) {
      if (print)
        acr_print_structure_and_related_scop(stdout, argv[i]);
      else
        acr_generate_code(argv[i], &build_options);
    }
  } else {
    fprintf(stderr, "No input file\nType \"%s -h\" for help\n", argv[-optind]);
  }

  return EXIT_SUCCESS;
}
