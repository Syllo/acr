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
#include <unistd.h>


static const char opt_options[] = "vho:";

int main(int argc, char** argv) {

  FILE* file = fopen("test.c", "r");

  for (;;) {
    int c = getopt(argc, argv, opt_options);
    if (c == -1) // No more candies
      break;

    switch (c) {
      case 'o':
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
        break;
    }
  }

  start_parsing(file);

  return EXIT_SUCCESS;
}
