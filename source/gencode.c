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

#include "acr/gencode.h"

#include <string.h>

#include "acr/acr_openscop.h"
#include "acr/print.h"
#include "acr/utils.h"

int start_acr_parsing(FILE* file, acr_compute_node* node_to_init);

static size_t acr_copy_from_file_to_file(FILE* input, FILE* output,
    size_t starting_position, size_t ending_position) {
  if (starting_position > ending_position)
    return 0;
  for (size_t i = starting_position; i < ending_position; ++i) {
    char li;
    if (fscanf(input, "%c", &li) == EOF)
      return i - starting_position;
    if (fprintf(output, "%c", li) != 1) {
      fseek(input, -1, SEEK_CUR);
      return i - starting_position;
    }
  }
  return ending_position - starting_position;
}

static size_t acr_position_of_init_in_node(acr_compute_node node) {
  acr_option_list list = acr_compute_node_get_option_list(node);
  unsigned long list_size = acr_compute_node_get_option_list_size(node);
  for (unsigned long i = 0; i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, list);
    if (acr_option_get_type(current_option) == acr_type_init) {
      return acr_init_get_pragma_position(current_option);
    }
  }
  return 0;
}

static size_t acr_skip_pragma_in_input(FILE* input) {
  size_t character_skipped = 1;
  char previous, current;
  if (fscanf(input, "%c", &previous) == EOF)
    return 0;

  if(previous == '\n')
    return 1;

  while (fscanf(input, "%c", &current) != EOF) {
    ++character_skipped;
    if (current == '\n' && previous != '\\') {
      break;
    }
    previous = current;
  }
  return character_skipped;
}

static char* acr_new_file_name(const char* initial_name) {
  size_t name_size = strlen(initial_name);
  char* new_file_name = malloc(name_size + 5 * sizeof(*new_file_name));
  acr_try_or_die(new_file_name == NULL, "malloc");
  strcpy(new_file_name, initial_name);

  size_t position_of_last_dot;
  for(position_of_last_dot = name_size; position_of_last_dot > 0; --position_of_last_dot) {
    if (initial_name[position_of_last_dot] == '.')
      break;
  }

  if (position_of_last_dot != 0) {
    for (size_t i = 0; i < name_size - position_of_last_dot + 1; ++i) {
      new_file_name[position_of_last_dot + i + 4] =
        new_file_name[position_of_last_dot + i];
    }
  } else {
    position_of_last_dot = name_size;
    new_file_name[position_of_last_dot + 4] = '\0';
  }
  new_file_name[position_of_last_dot] = '-';
  new_file_name[position_of_last_dot + 1] = 'a';
  new_file_name[position_of_last_dot + 2] = 'c';
  new_file_name[position_of_last_dot + 3] = 'r';
  return new_file_name;
}

static size_t acr_copy_from_file_avoiding_pragmas(FILE* input, FILE* output,
    size_t start, size_t stop, acr_compute_node all_pragmas) {
  fprintf(stderr, "Asked %zu %zu\n", start, stop);
  if (start > stop)
    return 0;
  acr_option_list list = acr_compute_node_get_option_list(all_pragmas);
  unsigned long list_size = acr_compute_node_get_option_list_size(all_pragmas);
  size_t current_position = start;
  for (unsigned long i = 0; current_position < stop && i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, list);
    size_t next_pragma_position = acr_option_get_pragma_position(current_option);
    if (current_position <= next_pragma_position) {
      if (current_position < next_pragma_position)
      current_position += acr_copy_from_file_to_file(input, output,
          current_position,
          next_pragma_position);
      if (current_position < stop)
        current_position += acr_skip_pragma_in_input(input);
    }
  }
  current_position += acr_copy_from_file_to_file(input, output, current_position,
      stop);
  fprintf(stderr, "Ended up %zu\n", current_position);
  return current_position;
}

void acr_print_init_function_declaration(FILE* out, const acr_option init) {
  fprintf(out, "void ");
  fprintf(out, "(*%s)(", acr_init_get_function_name(init));

  unsigned long num_parameters = acr_init_get_num_parameters(init);
  acr_parameter_declaration_list declaration_list =
    acr_init_get_parameter_list(init);
  for (unsigned long i = 0; i < num_parameters; ++i) {
    if (i != 0)
      fprintf(out, ", ");
    acr_parameter_specifier_list specifier_list =
      acr_parameter_declaration_get_specif_list(declaration_list,i);
    unsigned long num_specifiers =
      acr_parameter_declaration_get_num_specifiers(declaration_list, i);
    for (unsigned long j = 0; j < num_specifiers; ++j) {
      fprintf(out, "%s",
          acr_parameter_specifier_get_specifier(specifier_list, j));
      unsigned long pointer_depth =
        acr_parameter_specifier_get_pointer_depth(specifier_list, j);
      for (unsigned long k = 0; k < pointer_depth; ++k) {
        fprintf(out, "*");
      }
      fprintf(out, " ");
    }
    fprintf(out, "%s",
        acr_parameter_declaration_get_parameter_name(declaration_list,i));
  }
  fprintf(out, ") = %s;\n", acr_init_get_function_name(init));
}

void acr_print_node_initialization(FILE* out, const acr_compute_node node) {

  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  if (init == NULL) {
    fprintf(stderr, "Error no initialization in current node\n");
    pprint_acr_compute_node(stderr, node, 0);
    exit(EXIT_FAILURE);
  }
  acr_print_init_function_declaration(out, init);

}

void acr_generate_code(const char* filename) {
  FILE* current_file = fopen(filename, "r");
  if (current_file == NULL) {
    fprintf(stderr, "Unable to open file: %s\n", filename);
    perror("fopen");
    return;
  }

  acr_compute_node compute_node = NULL;
  start_acr_parsing(current_file, &compute_node);

  if (compute_node == NULL) {
    fprintf(stderr, "No pragma ACR found in file: %s\n", filename);
    return;
  }
  acr_compute_node all_options = acr_copy_compute_node(compute_node);
  acr_compute_node_list node_list =
    acr_new_compute_node_list_split_node(compute_node);

  if (node_list) {
    char* new_file_name = acr_new_file_name(filename);
    FILE* new_file = fopen(new_file_name, "w");
    if (!new_file) {
      perror("fopen");
      return;
    }
    acr_generate_preamble(new_file, filename);
    const unsigned long list_size = acr_compute_node_list_get_size(node_list);
    size_t position_in_input = 0;
    fseek(current_file, position_in_input, SEEK_SET);
    for (unsigned long i = 0; i < list_size; ++i) {
      acr_compute_node node = acr_compute_node_list_get_node(i, node_list);
      osl_scop_p scop = acr_extract_scop_in_compute_node(
          node, current_file, filename);
      if (scop) {
        if (scop->next) {
          osl_scop_free(scop->next);
          scop->next = NULL;
        }
        fseek(current_file, position_in_input, SEEK_SET);
        position_in_input = acr_copy_from_file_avoiding_pragmas(
            current_file, new_file, position_in_input,
            acr_position_of_init_in_node(node), all_options);
        unsigned long start, end;
        acr_scop_get_coordinates_start_end_kernel(node, scop, &start, &end);
        acr_print_node_initialization(new_file, node);
        osl_scop_free(scop);
      }
    }
    fseek(current_file, 0l, SEEK_END);
    long end_file = ftell(current_file);
    fseek(current_file, position_in_input, SEEK_SET);
    position_in_input = acr_copy_from_file_avoiding_pragmas(
        current_file, new_file, position_in_input,
        end_file, all_options);
    free(new_file_name);
    fclose(new_file);
  }

  acr_free_compute_node_list(node_list);
  acr_free_compute_node(all_options);
  fclose(current_file);
  /*
  char* buffer = NULL;
  size_t sizebuffer;
  acr_print_scop_to_buffer(scop, &buffer, &sizebuffer);
  fprintf(stderr, "%s", buffer);
  osl_scop_free(scop);
  scop = acr_read_scop_from_buffer(buffer, sizebuffer);
  osl_scop_print(stderr, scop);
  osl_scop_free(scop);
  free(buffer);
  */
}

void acr_generate_preamble(FILE* file, const char* filename) {
  fprintf(file,
      " /*\n"
      "   File automatically generated by ACR\n"
      "   Defunct input file: %s\n"
      "   Do not read this line.\n"
      "   Obviously you don't deserve dessert today by disobeying this basic order!\n"
      " */\n\n", filename);
      // # include here
}

void acr_print_structure_and_related_scop(FILE* out, const char* filename) {
  FILE* current_file = fopen(filename, "r");
  if (current_file == NULL) {
    fprintf(stderr, "Unable to open file: %s\n", filename);
    perror("fopen");
    return;
  }

  acr_compute_node compute_node = NULL;
  start_acr_parsing(current_file, &compute_node);

  if (compute_node == NULL) {
    fprintf(out, "No pragma ACR found in file: %s\n", filename);
  } else {

    acr_compute_node_list node_list =
      acr_new_compute_node_list_split_node(compute_node);
    if (node_list) {
      for(unsigned long j = 0; j < node_list->list_size; ++j) {
        acr_compute_node node = acr_compute_node_list_get_node(j, node_list);
        osl_scop_p scop = acr_extract_scop_in_compute_node(
            node, current_file, filename);
        if (scop) {
          if (scop->next) {
            osl_scop_free(scop->next);
            scop->next = NULL;
          }
          fprintf(out, "##### ACR options for node %lu: #####\n", j);
          pprint_acr_compute_node(out, node, 0ul);
          fprintf(out, "##### Scop for node %lu: #####\n", j);
          osl_scop_print(out, scop);
          osl_scop_free(scop);
        } else {
          fprintf(out, "##### No scop found for node %lu: #####", j);
          pprint_acr_compute_node(out, node, 0ul);
        }
        fprintf(out, "\n");
      }
    } else {
      fprintf(out, "No ACR node has been found in file: %s\n", filename);
    }
    acr_free_compute_node_list(node_list);
  }
  fclose(current_file);
}
