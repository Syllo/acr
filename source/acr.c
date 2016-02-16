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

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include "acr/print.h"
#include "acr/utils.h"
#include "acr/acr_openscop.h"

static const char opt_options[] = "vho:";

int main(int argc, char** argv) {

  char* output_file = NULL;

  for (;;) {
    int c = getopt(argc, argv, opt_options);
    if (c == -1) // No more candies
      break;

    switch (c) {
      case 'o':
        output_file = optarg;
        break;
      case 'h':
        fprintf(stdout, help);
          return EXIT_SUCCESS;
        break;
      case 'v':
        fprintf(stdout, "%s", version);
          return EXIT_SUCCESS;
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
      FILE* current_file = fopen(argv[i], "r");
      if (current_file == NULL) {
        fprintf(stderr, "Unable to open file: %s\n", argv[i]);
        perror("fopen");
        continue;
      }
      acr_compute_node compute_node = NULL;
      start_acr_parsing(current_file, &compute_node);
      if (compute_node == NULL) {
        fprintf(stdout, "No pragma acr found in %s\n",argv[i]);
      } else {
        fprintf(stdout, "Befor simplification\n");
        pprint_acr_compute_node(stdout, compute_node, 0);
        acr_compute_node_list node_list =
          acr_new_compute_node_list_split_node(compute_node);
        if (node_list) {
          fprintf(stdout, "After simplification\n");
          pprint_acr_compute_node_list(stdout, node_list, 0);
          for(unsigned int j = 0; j < node_list->list_size; ++j) {
            osl_scop_p scop = acr_extract_scop_in_compute_node(
                node_list->compute_node_list[j], current_file, argv[i]);
            osl_scop_print(stdout, scop);
          }
        }
        acr_free_compute_node_list(node_list);
      }
      fclose(current_file);
    }
  } else {
    fprintf(stderr, "No input file\nType \"%s -h\" for help\n", argv[-optind]);
  }

  return EXIT_SUCCESS;
}
