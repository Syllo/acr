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

#include <clan/scop.h>
#include <osl/extensions/coordinates.h>
#include <osl/extensions/arrays.h>
#include <osl/scop.h>
#include <string.h>

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
  if (start > stop)
    return 0;
  acr_option_list list = acr_compute_node_get_option_list(all_pragmas);
  unsigned long list_size = acr_compute_node_get_option_list_size(all_pragmas);
  size_t current_position = start;
  for (unsigned long i = 0; current_position < stop && i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, list);
    size_t next_pragma_position = acr_option_get_pragma_position(current_option);
    if (current_position <= next_pragma_position && next_pragma_position < stop) {
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
  return current_position;
}

static void acr_print_parameters(FILE* out, const acr_option init) {
  fprintf(out, "(");
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
  fprintf(out, ")");
}

static void acr_print_init_function_call(FILE* out, const acr_option init);

static void acr_print_acr_strategy_init(FILE* out,
    const acr_compute_node node) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  const char* fun_name = acr_init_get_function_name(init);
  unsigned long num_strategy = 0ul;
  unsigned long size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  for (unsigned long i = 0ul; i < size_list; ++i) {
    acr_option current_option = acr_option_list_get_option(i, options);
    if (acr_option_get_type(current_option) == acr_type_strategy) {
      ++num_strategy;
    }
  }
  fprintf(out, "  %s_runtime_data.num_strategy = %lu;\n",
      fun_name,
      num_strategy);
}

void acr_print_acr_runtime_init(FILE* out,
    const acr_compute_node node) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  const char* fun_name = acr_init_get_function_name(init);
  fprintf(out, "static struct acr_runtime_data %s_runtime_data;\n", fun_name);
  fprintf(out, "static void %s_acr_runtime_init", fun_name);
  acr_print_parameters(out, init);
  fprintf(out, " {\n"
      "  %s_runtime_data.osl_relation =\n"
      "    acr_read_scop_from_buffer(%s_acr_scop, %s_acr_scop_size);\n",
      fun_name, fun_name, fun_name);
  fprintf(out, "  %s_runtime_data.state =\n"
      "    cloog_state_malloc();\n",
      fun_name);
  fprintf(out, "  %s_runtime_data.cloog_input =\n"
      "    cloog_input_from_osl_scop(%s_runtime_data.state,\n"
      "      %s_runtime_data.osl_relation);\n",
      fun_name, fun_name, fun_name);
  fprintf(out, "  %s_runtime_data.context =\n"
      "    %s_runtime_data.cloog_input->context;\n",
      fun_name, fun_name);
  acr_print_acr_strategy_init(out, node);

  // Call function and change pointer to initial function
  fprintf(out, "  %s = %s_acr_initial;\n",
      fun_name, fun_name);
  fprintf(out, "  ");
  acr_print_init_function_call(out, init);
  fprintf(out, "}\n\n");
}


void acr_print_init_function_declaration(FILE* kernel_file, FILE* out,
    const acr_option init,
    unsigned long kernel_start, unsigned long kernel_end) {

  fprintf(out, "static void %s_acr_runtime_init",
      acr_init_get_function_name(init));
  acr_print_parameters(out, init);
  fprintf(out, ";\n");
  fprintf(out, "static void %s_acr_initial", acr_init_get_function_name(init));
  acr_print_parameters(out, init);
  fprintf(out, " {\n");
  fseek(kernel_file, kernel_start, SEEK_SET);
  acr_copy_from_file_to_file(kernel_file, out, kernel_start, kernel_end);
  fprintf(out, "\n}\n\n");

  fprintf(out, "static void (*%s)", acr_init_get_function_name(init));
  acr_print_parameters(out, init);
  fprintf(out, " = %s_acr_runtime_init;\n", acr_init_get_function_name(init));
}

void acr_print_node_initialization(FILE* in, FILE* out,
    const acr_compute_node node,
    unsigned long kernel_start, unsigned long kernel_end) {

  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  if (init == NULL) {
    fprintf(stderr, "Error no initialization in current node\n");
    pprint_acr_compute_node(stderr, node, 0);
    exit(EXIT_FAILURE);
  }
  acr_print_init_function_declaration(in, out, init,
      kernel_start, kernel_end);

}

void acr_print_node_init_function_call(FILE* out,
    const acr_compute_node node) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  acr_print_init_function_call(out, init);
}

static void acr_print_init_function_call(FILE* out, const acr_option init) {
  fprintf(out, "%s(", acr_init_get_function_name(init));
  unsigned long num_parameters = acr_init_get_num_parameters(init);
  acr_parameter_declaration_list declaration_list =
    acr_init_get_parameter_list(init);
  if (num_parameters != 1 ||
      (num_parameters == 1 && strcmp(
        acr_parameter_declaration_get_parameter_name(declaration_list, 0ul),
        "void") != 0))
  for (unsigned long i = 0; i < num_parameters; ++i) {
    if (i != 0)
      fprintf(out, ", ");
    fprintf(out, "%s",
        acr_parameter_declaration_get_parameter_name(declaration_list,i));
  }
  fprintf(out, ");\n");
}

static char* acr_get_scop_prefix(const acr_compute_node node) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  return acr_init_get_function_name(init);
}

void acr_print_scop_in_file(FILE* output,
    const char* scop_prefix, osl_scop_p scop) {
  char* buffer = NULL;
  osl_generic_remove(&scop->extension, OSL_URI_COORDINATES);
  size_t sizebuffer;
  acr_print_scop_to_buffer(scop, &buffer, &sizebuffer);
  fprintf(output, "static size_t %s_acr_scop_size = %zu;\n", scop_prefix,
      sizebuffer);
  fprintf(output, "static char %s_acr_scop[] = \"", scop_prefix);
  size_t current_position = 0;
  while (buffer[current_position] != '\0') {
    if (buffer[current_position] == '\n') {
      fprintf(output, "\\n\"\n\"");
    } else {
      fprintf(output, "%c", buffer[current_position]);
    }
    ++current_position;
  }
  fprintf(output, "\";\n\n");
  free(buffer);
}

void acr_print_destroy(FILE* output, const acr_compute_node node) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  fprintf(output, "free_acr_runtime_data(&%s_runtime_data);\n",
      acr_init_get_function_name(init));
}

void acr_print_scanning_function(FILE* out, const acr_compute_node node,
    osl_scop_p scop) {
  osl_generic_remove(&scop->extension, OSL_URI_ARRAYS);
  acr_option monitor =
    acr_compute_node_get_option_of_type(acr_type_monitor, node, 1);
  if (monitor == NULL) {
    fprintf(stderr, "No data to monitor\n");
    exit(1);
  }
  osl_scop_p newscop = acr_openscop_gen_monitor_loop(monitor, scop);
  if (newscop == NULL) {
    fprintf(stderr, "It is not possible to find monitor data boundaries\n");
    exit(1);
  }
  fprintf(out, "");
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
        const char* scop_prefix = acr_get_scop_prefix(node);
        unsigned long kernel_start, kernel_end;
        acr_scop_coord_to_acr_coord(current_file, scop, node,
            &kernel_start, &kernel_end);

        fseek(current_file, position_in_input, SEEK_SET);
        position_in_input = acr_copy_from_file_avoiding_pragmas(
            current_file, new_file, position_in_input,
            acr_position_of_init_in_node(node), all_options);

        acr_print_scop_in_file(new_file, scop_prefix, scop);

        acr_print_node_initialization(current_file, new_file, node,
            kernel_start, kernel_end);

        acr_print_scanning_function(new_file, node, scop);
        acr_print_acr_runtime_init(new_file, node);

        fseek(current_file, position_in_input, SEEK_SET);
        position_in_input = acr_copy_from_file_avoiding_pragmas(
            current_file,
            new_file,
            position_in_input, kernel_start,
            all_options);

        fprintf(new_file, "/* Do acr stuff here */\n");
        acr_print_node_init_function_call(new_file, node);
        position_in_input = kernel_end;
        fseek(current_file, position_in_input, SEEK_SET);
        osl_scop_free(scop);

        acr_option destroy =
          acr_compute_node_get_option_of_type(acr_type_destroy, node, 1);
        size_t destroy_pos = acr_destroy_get_pragma_position(destroy);
        position_in_input = acr_copy_from_file_avoiding_pragmas(
            current_file,
            new_file,
            position_in_input, destroy_pos,
            all_options);
        acr_print_destroy(new_file, node);
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
}

void acr_generate_preamble(FILE* file, const char* filename) {
  fprintf(file,
      " /*\n"
      "   File automatically generated by ACR\n"
      "   Defunct input file: %s\n"
      "   Do not read this line.\n"
      "   Obviously you don't deserve dessert today by disobeying this basic order!\n"
      " */\n\n"
      "#include <acr/acr_runtime.h>\n\n", filename);
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

osl_scop_p acr_extract_scop_in_compute_node(const acr_compute_node node,
                                            FILE* input_file,
                                            const char* name_input_file) {

  clan_options_t clan_options;

  clan_options.autoscop = 1;
  clan_options.bounded_context = 0;
  clan_options.castle = 0;
  clan_options.extbody = 0;
  clan_options.inputscop = 0;
  clan_options.name = acr_strdup(name_input_file);
  clan_options.noloopcontext = 0;
  clan_options.nosimplify = 0;
  clan_options.outscoplib = 0;
  clan_options.precision = 0;
  clan_options.structure = 0;

  unsigned long start, end;
  acr_get_start_and_stop_for_clan(node, &start, &end);

  osl_scop_p scop = clan_scop_extract_delimited(input_file,
      &clan_options, start, end);
  free(clan_options.name);
  return scop;
}

void acr_get_start_and_stop_for_clan(const acr_compute_node node,
    unsigned long* start, unsigned long *stop) {

  *start = 0ul;
  *stop = 0ul;

  unsigned long option_list_size = acr_compute_node_get_option_list_size(node);
  const acr_option_list option_list = acr_compute_node_get_option_list(node);

  for (unsigned int i = 0ul; i < option_list_size; ++i) {
    const acr_option option = acr_option_list_get_option(i, option_list);
    switch (acr_option_get_type(option)) {
      case acr_type_init:
        *start = acr_init_get_pragma_position(option);
        *stop = *start;
        break;
      case acr_type_alternative:
        *start = acr_alternative_get_pragma_position(option);
        break;
      case acr_type_grid:
        *start = acr_grid_get_pragma_position(option);
        break;
      case acr_type_monitor:
        *start = acr_monitor_get_pragma_position(option);
        break;
      case acr_type_strategy:
        *start = acr_strategy_get_pragma_position(option);
        break;
      case acr_type_destroy:
        *stop = acr_destroy_get_pragma_position(option);
      case acr_type_unknown:
        break;
    }
  }
}

void acr_scop_coord_to_acr_coord(
    FILE* kernel_file,
    const osl_scop_p scop,
    const acr_compute_node compute_node,
    unsigned long* start,
    unsigned long* end) {
  acr_get_start_and_stop_for_clan(compute_node, start, end);
  osl_coordinates_p osl_coord = osl_generic_lookup(scop->extension,
      OSL_URI_COORDINATES);
  if (osl_coord == NULL)
    return;
  fseek(kernel_file, *start, SEEK_SET);
  unsigned long line_num = 1;
  char c;
  unsigned long current_position = *start;
  while (line_num < (unsigned long) osl_coord->line_start) {
    if(fscanf(kernel_file, "%c", &c) == EOF)
      break;
    ++current_position;
    if (c == '\n')
      ++line_num;
  }
  if (osl_coord->column_start > 0) {
    current_position += osl_coord->column_start - 1;
    fseek(kernel_file, osl_coord->column_start - 1, SEEK_CUR);
  }
  *start = current_position;
  while (line_num < (unsigned long) osl_coord->line_end) {
    if(fscanf(kernel_file, "%c", &c) == EOF)
      break;
    ++current_position;
    if (c == '\n')
      ++line_num;
  }
  if (osl_coord->column_end > 0)
    current_position += osl_coord->column_end;
  *end = current_position;
}
