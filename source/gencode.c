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

#include <limits.h>
#include <string.h>

#include <clan/scop.h>
#include <cloog/cloog.h>
#include <isl/constraint.h>
#include <isl/printer.h>
#include <osl/extensions/coordinates.h>
#include <osl/extensions/arrays.h>
#include <osl/scop.h>
#include <osl/strings.h>

#include "acr/print.h"
#include "acr/utils.h"

int start_acr_parsing(FILE* file, acr_compute_node* node_to_init);

static char* acr_get_scop_prefix(const acr_compute_node node) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  return acr_init_get_function_name(init);
}

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

static unsigned long acr_strategy_get_min(const acr_option strategy) {
  long values[2];
  acr_strategy_get_int_val(strategy, values);
  return values[0];
}

static unsigned long acr_strategy_get_max(const acr_option strategy) {
  long values[2];
  acr_strategy_get_int_val(strategy, values);
  switch (acr_strategy_get_strategy_type(strategy)) {
    case acr_strategy_direct:
      return values[0];
      break;
    case acr_strategy_range:
      return values[1];
      break;
    case acr_strategy_unknown:
      break;
  }
  return 0;
}

static void acr_populate_strategy_and_alternative_list(
    const acr_compute_node node,
    unsigned long strategy_list_size,
    acr_option_list strategy_list,
    unsigned long alternative_list_size,
    acr_option_list alternative_list,
    unsigned long *strategy_to_alternative_index) {

  unsigned long size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  unsigned long current_strategy_position = 0ul;
  unsigned long current_alternative_position = 0ul;
  for (unsigned long i = 0ul; i < size_list; ++i) {
    acr_option current_option = acr_option_list_get_option(i, options);
    if (acr_option_get_type(current_option) == acr_type_strategy) {
      acr_option_list_set_option(current_option, current_strategy_position,
          strategy_list);
      current_strategy_position += 1;
    }
    if (acr_option_get_type(current_option) == acr_type_alternative) {
      acr_option_list_set_option(current_option, current_alternative_position,
          alternative_list);
      current_alternative_position += 1;
    }
  }

  // sort strategies
  bool moved = true;
  for (unsigned long i = 0; moved && i < strategy_list_size - 1; ++i) {
    moved = false;
    for (unsigned long j = 0; j < strategy_list_size - 1 - i; ++j) {
      acr_option current = acr_option_list_get_option(j, strategy_list);
      acr_option next = acr_option_list_get_option(j+1, strategy_list);
      unsigned long max_current = acr_strategy_get_max(current);
      unsigned long min_next = acr_strategy_get_min(next);
      if (min_next < max_current) {
        acr_option_list_set_option(next, j, strategy_list);
        acr_option_list_set_option(current, j+1, strategy_list);
        moved = true;
      }
    }
  }

  for (unsigned long i = 0; i < strategy_list_size; ++i) {
    acr_option strategy = acr_option_list_get_option(i, strategy_list);
    for (unsigned long j = 0; j < alternative_list_size; ++j) {
      acr_option alternative = acr_option_list_get_option(j, alternative_list);
      if (acr_strategy_correspond_to_alternative(strategy, alternative)) {
        strategy_to_alternative_index[i] = j;
      }
    }
  }
}

void acr_print_acr_alternatives(FILE *out,
      const char *prefix,
      unsigned long num_alternatives,
      const acr_option_list alternative_list,
      const osl_scop_p scop) {
  static const char* alternative_types_char[] = {
    [acr_alternative_function] = "acr_runtime_alternative_function",
    [acr_alternative_parameter] = "acr_runtime_alternative_parameter"};
  osl_strings_p parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);
  fprintf(out, "static struct runtime_alternative %s_alternatives[] = {\n",
      prefix);
  for (unsigned long i = 0; i < num_alternatives; ++i) {
    const acr_option alternative =
      acr_option_list_get_option(i, alternative_list);
    enum acr_alternative_type alternative_type =
      acr_alternative_get_type(alternative);
    if (i > 0) {
      fprintf(out, ",\n");
    }
    fprintf(out, "  [%ld] = { .type = %s, .alternative_number = %luul, ",
        i,
        alternative_types_char[alternative_type], i);
    const char *name = acr_alternative_get_object_to_swap_name(alternative);
    switch (alternative_type) {
      case acr_alternative_parameter:
        fprintf(out,
            ".value = { .alt.parameter.parameter_value = %ldl"
            " , .alt.parameter.parameter_position = %luul"
            " , .name_to_swap = \"%s\" } ",
            acr_alternative_get_replacement_parameter(alternative),
            osl_strings_find(parameters, name),
            name);
        break;
      case acr_alternative_function:
        fprintf(out,
            ".value = { .alt.function_to_swap = %s"
            " , .name_to_swap = \"%s\" } ",
            acr_alternative_get_replacement_function(alternative),
            name);
        break;
      case acr_alternative_unknown:
        break;
    }
    fprintf(out, "}");
  }
  fprintf(out, "\n};\n");
}

static void acr_print_get_alternetive_from_val(
    FILE *out,
    const char* prefix,
    unsigned long strategy_list_size,
    acr_option_list strategy_list,
    unsigned long *strategy_to_alternative_index) {

  fprintf(out, "static struct runtime_alternative *%s_alternative_fun[%u] = {\n",
      prefix, UCHAR_MAX);
  long max_current;
  long min_current;
  bool not_first_alternative = false;
  for (unsigned long i = 0; i < strategy_list_size; ++i) {
      acr_option current = acr_option_list_get_option(i, strategy_list);
      max_current = acr_strategy_get_max(current);
      min_current = acr_strategy_get_min(current);
      for (long j = min_current; j <= max_current; ++j) {
        if (not_first_alternative)
          fprintf(out, ", ");
        else
          not_first_alternative = true;
        fprintf(out, "&%s_alternatives[%lu]", prefix,
            strategy_to_alternative_index[i]);
      }
  }
  fprintf(out, "};\n");
  fprintf(out, "struct runtime_alternative* %s_get_alternative_from_val(\n"
      "    acr_monitored_data data) {\n"
      "  return %s_alternative_fun[data];\n"
      "}\n", prefix, prefix);
}

bool acr_print_acr_alternative_and_strategy_init(FILE* out,
    const acr_compute_node node,
    const osl_scop_p scop) {
  const char* prefix = acr_get_scop_prefix(node);
  unsigned long num_strategy = 0ul;
  unsigned long num_alternatives = 0ul;
  unsigned long size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  for (unsigned long i = 0ul; i < size_list; ++i) {
    acr_option current_option = acr_option_list_get_option(i, options);
    if (acr_option_get_type(current_option) == acr_type_strategy) {
      ++num_strategy;
    }
    if (acr_option_get_type(current_option) == acr_type_alternative) {
      ++num_alternatives;
    }
  }
  if (num_strategy == 0ul) {
    fprintf(stderr,
        "[ACR] Warning: There is no strategies left for this node:\n");
    pprint_acr_compute_node(stderr, node, 0);
    return false;
  }
  if (num_alternatives == 0ul) {
    fprintf(stderr,
        "[ACR] Warning: There is no alternatives left for this node:\n");
    pprint_acr_compute_node(stderr, node, 0);
    return false;
  }
  acr_option_list strategy_list = acr_new_option_list(num_strategy);
  unsigned long *strategy_to_alternative_index =
    malloc(num_strategy * sizeof(*strategy_to_alternative_index));
  acr_option_list alternative_list = acr_new_option_list(num_alternatives);

  acr_populate_strategy_and_alternative_list(
      node,
      num_strategy,
      strategy_list,
      num_alternatives,
      alternative_list,
      strategy_to_alternative_index);

  acr_print_acr_alternatives(out,
      prefix,
      num_alternatives,
      alternative_list,
      scop);

  acr_print_get_alternetive_from_val(out,
      prefix, num_strategy, strategy_list, strategy_to_alternative_index);

  free(strategy_list);
  free(strategy_to_alternative_index);
  free(alternative_list);

  return true;
}

void acr_print_isl_lex_min_max_bound(FILE *out,
    bool upper_bound,
    isl_set *set,
    unsigned long dim_to_print,
    const osl_strings_p parameters) {
  const char max_char[] = "__acr__max__";
  const char min_char[] = "__acr__min__";
  const char *aggregator = upper_bound ? max_char : min_char;
  unsigned long num_dim = isl_set_n_dim(set);
  unsigned long num_param = isl_set_n_param(set);
  isl_set *only_dim_wanted = isl_set_copy(set);
  if (dim_to_print+1 < num_dim)
    only_dim_wanted = isl_set_project_out(only_dim_wanted, isl_dim_set,
        dim_to_print+1, num_dim - dim_to_print - 1);
  if (dim_to_print != 0)
    only_dim_wanted = isl_set_project_out(only_dim_wanted, isl_dim_set,
        0ul, dim_to_print);
  only_dim_wanted = isl_set_coalesce(only_dim_wanted);
  isl_basic_set_list *bset_list = isl_set_get_basic_set_list(only_dim_wanted);
  isl_set_free(only_dim_wanted);
  int num_basic_set = isl_basic_set_list_n_basic_set(bset_list);
  int remaining_basic_set = num_basic_set;
  isl_printer *printer = isl_printer_to_file(isl_set_get_ctx(set), out);
  for (int i = 0; i < num_basic_set; ++i, --remaining_basic_set) {
    if (remaining_basic_set > 1) {
      fprintf(out, "%s(", aggregator);
    }
    isl_basic_set *bset = isl_basic_set_list_get_basic_set(bset_list, i);
    isl_constraint_list *clist = isl_basic_set_get_constraint_list(bset);
    int num_constraints = isl_constraint_list_n_constraint(clist);
    for (int j = 0; j < num_constraints; ++j) {
      isl_constraint *co = isl_constraint_list_get_constraint(clist, j);
      if (isl_constraint_involves_dims(co, isl_dim_set, 0, 1)) {
        isl_val *dim_val =
          isl_constraint_get_constant_val(co);
        dim_val = isl_val_neg(dim_val);
        dim_val = isl_val_add_ui(dim_val, 1ul);
        isl_printer_print_val(printer, dim_val);
        isl_printer_flush(printer);
        isl_val_free(dim_val);
        for (unsigned long k = 0; k < num_param; ++k) {
          if (isl_constraint_involves_dims(co, isl_dim_param, k, 1)) {
            fprintf(out, " + %s*", parameters->string[k]);
            dim_val =
              isl_constraint_get_coefficient_val(co, isl_dim_param, k);
            dim_val = isl_val_neg(dim_val);
            isl_printer_print_val(printer, dim_val);
            isl_printer_flush(printer);
            isl_val_free(dim_val);
          }
        }
      }
      isl_constraint_free(co);
    }
    isl_basic_set_free(bset);
    isl_constraint_list_free(clist);
    if (remaining_basic_set > 1) {
      fprintf(out, ", ");
    }
  }
  isl_basic_set_list_free(bset_list);
  remaining_basic_set = num_basic_set;
  for (int i = 0; i < num_basic_set; ++i, --remaining_basic_set) {
    if (remaining_basic_set > 1) {
      fprintf(out, ")");
    }
  }
  isl_printer_free(printer);
}

static void acr_print_monitor_max_dims(FILE *out,
    const char *prefix,
    dimensions_upper_lower_bounds *bounds,
    const osl_scop_p scop) {
  unsigned long num_monitor_dims = 0;
  for (unsigned long i = 0; i < bounds->num_dimensions; ++i) {
    if (bounds->dimensions_type[i] == acr_dimension_type_bound_to_monitor)
      num_monitor_dims++;
  }
  osl_strings_p parameters = osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);

  fprintf(out,
      "  %s_runtime_data.monitor_dim_max =\n"
      "    malloc(%luul * sizeof(*%s_runtime_data.monitor_dim_max));\n"
      "#ifdef __acr__max__\n"
      "#undef __acr__max__\n"
      "#endif\n"
      "#define __acr__max__(a,b) (((a)>(b))?(a):(b))\n",
      prefix, num_monitor_dims, prefix);

  unsigned long current_dim = 0;
  for (unsigned long i = 0; i < bounds->num_dimensions; ++i) {
    if (bounds->dimensions_type[i] == acr_dimension_type_bound_to_monitor) {
      fprintf(out, "  %s_runtime_data.monitor_dim_max[%lu] = ",
          prefix, current_dim++);
      acr_print_isl_lex_min_max_bound(out,
          true, bounds->bound_lexmax, i, parameters);
      fprintf(out, ";\n");
    }
  }
  fprintf(out, "#undef __acr__max__\n");
}

void acr_print_acr_runtime_init(FILE* out,
    const acr_compute_node node,
    unsigned long num_parameters,
    dimensions_upper_lower_bounds *bounds,
    const osl_scop_p scop) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  const char* prefix = acr_init_get_function_name(init);
  unsigned long num_alternatives = 0ul;

  unsigned long size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  for (unsigned long i = 0ul; i < size_list; ++i) {
    acr_option current_option = acr_option_list_get_option(i, options);
    if (acr_option_get_type(current_option) == acr_type_alternative) {
      ++num_alternatives;
    }
  }
  unsigned long num_monitor_dims = 0;
  for (unsigned long i = 0; i < bounds->num_dimensions; ++i) {
    if (bounds->dimensions_type[i] == acr_dimension_type_bound_to_monitor)
      num_monitor_dims++;
  }

  acr_option grid = acr_compute_node_get_option_of_type(acr_type_grid, node, 1);
  fprintf(out, "static struct acr_runtime_data %s_runtime_data =\n"
      "    { .num_alternatives = %luul, "
      ".alternatives = %s_alternatives, "
      ".num_parameters = %luul, "
      ".num_monitor_dims = %luul, "
      ".grid_size = %luul };\n",
      prefix, num_alternatives, prefix,
      num_parameters, num_monitor_dims, acr_grid_get_grid_size(grid));

  fprintf(out, "static void %s_acr_runtime_init", prefix);
  acr_print_parameters(out, init);
  fprintf(out, " {\n");

  acr_print_monitor_max_dims(out, prefix, bounds, scop);

  fprintf(out,
      "  init_acr_runtime_data(\n"
      "      &%s_runtime_data,\n"
      "      %s_acr_scop,\n"
      "      %s_acr_scop_size);\n",
      prefix, prefix, prefix);

  fprintf(out,
      "  acr_cloog_init_alternative_constraint_from_cloog_union_domain(\n"
      "      &%s_runtime_data);\n", prefix);

  // Call function and change pointer to initial function
  fprintf(out, "  %s = %s_acr_initial;\n  ",
      prefix, prefix);
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

bool acr_print_node_initialization(FILE* in, FILE* out,
    const acr_compute_node node,
    unsigned long kernel_start, unsigned long kernel_end) {

  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  if (init == NULL) {
    fprintf(stderr, "Error no initialization in current node\n");
    pprint_acr_compute_node(stderr, node, 0);
    return false;
  }
  acr_print_init_function_declaration(in, out, init,
      kernel_start, kernel_end);
  return true;

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

bool acr_print_scanning_function(FILE* out, const acr_compute_node node,
    osl_scop_p scop,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used) {
  osl_generic_remove(&scop->extension, OSL_URI_ARRAYS);
  acr_option monitor =
    acr_compute_node_get_option_of_type(acr_type_monitor, node, 1);
  if (!monitor) {
    fprintf(stderr, "[ACR] error: The current node has no data to monitor\n");
    return false;
  }
  acr_option grid =
    acr_compute_node_get_option_of_type(acr_type_grid, node, 1);
  if (!grid) {
    fprintf(stderr, "[ACR] error: The current node has no grid info\n");
    return false;
  }
  unsigned long grid_size = acr_grid_get_grid_size(grid);
  if (monitor == NULL) {
    fprintf(stderr, "No data to monitor\n");
    return false;
  }
  const char *prefix = acr_get_scop_prefix(node);
  osl_scop_p new_scop = acr_openscop_gen_monitor_loop(monitor, scop,
      grid_size, dims, bound_used);
  if (new_scop == NULL) {
    fprintf(stderr, "It is not possible to find monitor data boundaries\n");
    return false;
  }

  fprintf(out, "void %s_monitoring_function(void) {\n", prefix);
  switch (acr_monitor_get_function(monitor)) {
    case acr_monitor_function_min:
    case acr_monitor_function_max:
      break;
    case acr_monitor_function_avg:
      fprintf(out, "size_t temp_avg, num_value;\n");
      break;
    case acr_monitor_function_unknown:
      break;
  }

  CloogState *cloog_state = cloog_state_malloc();
  CloogInput *cloog_input = cloog_input_from_osl_scop(cloog_state, new_scop);
  CloogOptions *cloog_option = cloog_options_malloc(cloog_state);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->scop = new_scop;
  cloog_option->otl = 1;
  cloog_option->language = 0;
  CloogProgram *cloog_program = cloog_program_alloc(cloog_input->context,
      cloog_input->ud, cloog_option);
  cloog_program = cloog_program_generate(cloog_program, cloog_option);

  cloog_program_pprint(out, cloog_program, cloog_option);
  fprintf(out, "}\n");

  cloog_program_free(cloog_program);
  cloog_state_free(cloog_state);
  cloog_options_free(cloog_option);
  free(cloog_input);
  return true;
}

static void acr_delete_alternative_parameters_where_parameter_not_present_in_scop(
    acr_compute_node node,
    const osl_scop_p scop) {

  osl_strings_p parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);
  size_t total_parameters = osl_strings_size(parameters);

  for (unsigned int i = 0ul; i < acr_compute_node_get_option_list_size(node); ++i) {
    const acr_option_list option_list = acr_compute_node_get_option_list(node);
    const acr_option option = acr_option_list_get_option(i, option_list);
    if (acr_option_get_type(option) == acr_type_alternative) {
      if (acr_alternative_get_type(option) == acr_alternative_parameter) {
        const char* alternate_param =
          acr_alternative_get_object_to_swap_name(option);
        if (osl_strings_find(parameters, alternate_param) >= total_parameters) {
          fprintf(stderr,
              "[ACR] Warning: The parameter %s is used in the computation"
              " kernel\n"
              "               The following alternative will be ignored:\n",
              alternate_param);
          pprint_acr_option(stderr, option, 0);
          acr_compute_node_delete_option_from_position(i, node);
          i -= 1ul;
        }
      }
    }
  }
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
        acr_delete_alternative_parameters_where_parameter_not_present_in_scop(
            node,
            scop);
        while(acr_simplify_compute_node(node));

        dimensions_upper_lower_bounds_all_statements *bounds_all =
          acr_osl_get_upper_lower_bound_all(scop->statement);
        if(!acr_osl_find_and_verify_free_dims_position(node, scop, bounds_all)){
          acr_osl_free_dimension_upper_lower_bounds_all(bounds_all);
          osl_scop_free(scop);
          continue;
        }

        char *buffer;
        size_t size_buffer;
        FILE* temp_buffer = open_memstream(&buffer, &size_buffer);
        const size_t scop_start_position = position_in_input;
        const char* scop_prefix = acr_get_scop_prefix(node);
        unsigned long kernel_start, kernel_end;
        acr_scop_coord_to_acr_coord(current_file, scop, node,
            &kernel_start, &kernel_end);

        fseek(current_file, position_in_input, SEEK_SET);
        position_in_input = acr_copy_from_file_avoiding_pragmas(
            current_file, temp_buffer, position_in_input,
            acr_position_of_init_in_node(node), all_options);

        acr_print_scop_in_file(temp_buffer, scop_prefix, scop);

        if (!acr_print_node_initialization(current_file, temp_buffer, node,
            kernel_start, kernel_end)) {
          fclose(temp_buffer);
          free(buffer);
          position_in_input = scop_start_position;
          osl_scop_free(scop);
          continue;
        }

        dimensions_upper_lower_bounds *bound_used;
        if (!acr_print_scanning_function(temp_buffer, node, scop, bounds_all,
              &bound_used)) {
          fclose(temp_buffer);
          free(buffer);
          position_in_input = scop_start_position;
          osl_scop_free(scop);
          acr_osl_free_dimension_upper_lower_bounds_all(bounds_all);
          continue;
        }
        if(!acr_print_acr_alternative_and_strategy_init(temp_buffer, node, scop)) {
          fclose(temp_buffer);
          free(buffer);
          position_in_input = scop_start_position;
          osl_scop_free(scop);
          acr_osl_free_dimension_upper_lower_bounds_all(bounds_all);
          continue;
        }
        acr_print_acr_runtime_init(temp_buffer, node,
            scop->context->nb_parameters, bound_used, scop);

        fseek(current_file, position_in_input, SEEK_SET);
        position_in_input = acr_copy_from_file_avoiding_pragmas(
            current_file,
            temp_buffer,
            position_in_input, kernel_start,
            all_options);

        fprintf(temp_buffer, "/* Do acr stuff here */\n");
        acr_print_node_init_function_call(temp_buffer, node);
        position_in_input = kernel_end;
        fseek(current_file, position_in_input, SEEK_SET);
        osl_scop_free(scop);

        acr_option destroy =
          acr_compute_node_get_option_of_type(acr_type_destroy, node, 1);
        size_t destroy_pos = acr_destroy_get_pragma_position(destroy);
        position_in_input = acr_copy_from_file_avoiding_pragmas(
            current_file,
            temp_buffer,
            position_in_input, destroy_pos,
            all_options);
        acr_print_destroy(temp_buffer, node);
        fclose(temp_buffer);
        fprintf(new_file, "%s", buffer);
        free(buffer);
        acr_osl_free_dimension_upper_lower_bounds_all(bounds_all);
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
          acr_delete_alternative_parameters_where_parameter_not_present_in_scop(
              node,
              scop);
          while(acr_simplify_compute_node(node));
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
