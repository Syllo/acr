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

#include <inttypes.h>
#include <limits.h>
#include <string.h>

#include <acr/acr_runtime_code_generation.h>
#include <clan/scop.h>
#include <cloog/cloog.h>
#include <cloog/isl/domain.h>
#include <isl/constraint.h>
#include <isl/printer.h>
#include <osl/extensions/arrays.h>
#include <osl/extensions/coordinates.h>
#include <osl/extensions/scatnames.h>
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
  size_t list_size = acr_compute_node_get_option_list_size(node);
  for (size_t i = 0; i < list_size; ++i) {
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
    size_t start, size_t stop, const acr_compute_node all_pragmas) {
  if (start > stop)
    return 0;
  acr_option_list list = acr_compute_node_get_option_list(all_pragmas);
  size_t list_size = acr_compute_node_get_option_list_size(all_pragmas);
  size_t current_position = start;
  for (size_t i = 0; current_position < stop && i < list_size; ++i) {
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
  size_t num_parameters = acr_init_get_num_parameters(init);
  acr_parameter_declaration_list declaration_list =
    acr_init_get_parameter_list(init);
  for (size_t i = 0; i < num_parameters; ++i) {
    if (i != 0)
      fprintf(out, ", ");
    acr_parameter_specifier_list specifier_list =
      acr_parameter_declaration_get_specif_list(declaration_list,i);
    size_t num_specifiers =
      acr_parameter_declaration_get_num_specifiers(declaration_list, i);
    for (size_t j = 0; j < num_specifiers; ++j) {
      fprintf(out, "%s",
          acr_parameter_specifier_get_specifier(specifier_list, j));
      size_t pointer_depth =
        acr_parameter_specifier_get_pointer_depth(specifier_list, j);
      for (size_t k = 0; k < pointer_depth; ++k) {
        fprintf(out, "*");
      }
      fprintf(out, " ");
    }
    fprintf(out, "%s",
        acr_parameter_declaration_get_parameter_name(declaration_list,i));
  }
  fprintf(out, ")");
}

static void acr_print_init_function_call(FILE* out, const acr_option init,
    const struct acr_build_options *options);

static intmax_t acr_strategy_get_min(const acr_option strategy) {
  intmax_t values[2];
  acr_strategy_get_int_val(strategy, values);
  return values[0];
}

static intmax_t acr_strategy_get_max(const acr_option strategy) {
  intmax_t values[2];
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
    size_t strategy_list_size,
    acr_option_list strategy_list,
    size_t alternative_list_size,
    acr_option_list alternative_list,
    size_t *strategy_to_alternative_index) {

  size_t size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  size_t current_strategy_position = 0;
  size_t current_alternative_position = 0;
  for (size_t i = 0; i < size_list; ++i) {
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
  for (size_t i = 0; moved && i < strategy_list_size - 1; ++i) {
    moved = false;
    for (size_t j = 0; j < strategy_list_size - 1 - i; ++j) {
      acr_option current = acr_option_list_get_option(j, strategy_list);
      acr_option next = acr_option_list_get_option(j+1, strategy_list);
      intmax_t max_current = acr_strategy_get_max(current);
      intmax_t min_next = acr_strategy_get_min(next);
      if (min_next < max_current) {
        acr_option_list_set_option(next, j, strategy_list);
        acr_option_list_set_option(current, j+1, strategy_list);
        moved = true;
      }
    }
  }

  for (size_t i = 0; i < strategy_list_size; ++i) {
    acr_option strategy = acr_option_list_get_option(i, strategy_list);
    for (size_t j = 0; j < alternative_list_size; ++j) {
      acr_option alternative = acr_option_list_get_option(j, alternative_list);
      if (acr_strategy_correspond_to_alternative(strategy, alternative)) {
        strategy_to_alternative_index[i] = j;
      }
    }
  }
}

static void sort_osl_strings(osl_strings_p a, osl_strings_p b) {
  size_t size_a = osl_strings_size(a);
  for (size_t end = size_a-1; end < size_a; --end) {
    for (size_t to_end = 0; to_end < end; ++to_end) {
      if (osl_strings_find(b, a->string[to_end]) > osl_strings_find(b, a->string[to_end+1])) {
        char *tmp = a->string[to_end];
        a->string[to_end] = a->string[to_end+1];
        a->string[to_end+1] = tmp;
      }
    }
  }
}

static void acr_print_acr_alternatives(FILE *out,
      const char *prefix,
      size_t num_alternatives,
      const acr_option_list alternative_list,
      const osl_scop_p scop,
      const struct acr_build_options *options) {

  osl_strings_p pragma_alternative_parameters = osl_strings_malloc();
  size_t string_size = 0;
  for (size_t i = 0; i < num_alternatives; ++i) {
    acr_option current_option =
      acr_option_list_get_option(i, alternative_list);
    if (acr_option_get_type(current_option) == acr_type_alternative &&
        acr_alternative_get_type(current_option) == acr_alternative_parameter) {
      const char *parameter_to_swap =
        acr_alternative_get_object_to_swap_name(current_option);
      if (osl_strings_find(pragma_alternative_parameters, parameter_to_swap)
          == string_size) {
        osl_strings_add(pragma_alternative_parameters, parameter_to_swap);
        string_size += 1;
      }
    }
  }
  osl_strings_p parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);

  sort_osl_strings(pragma_alternative_parameters, parameters);

  static const char*const alternative_types_char[] = {
    [acr_alternative_function] = "acr_runtime_alternative_function",
    [acr_alternative_parameter] = "acr_runtime_alternative_parameter",
    [acr_alternative_zero_computation] =
      "acr_runtime_alternative_zero_computation",
    [acr_alternative_corner_computation] =
      "acr_runtime_alternative_corner_computation",
    [acr_alternative_full_computation] =
      "acr_runtime_alternative_full_computation"};
  fprintf(out, "static struct runtime_alternative %s_alternatives[] = {\n",
      prefix);
  size_t num_function = 0;
  for (size_t i = 0; i < num_alternatives; ++i) {
    const acr_option alternative =
      acr_option_list_get_option(i, alternative_list);
    enum acr_alternative_type alternative_type =
      acr_alternative_get_type(alternative);
    if (i > 0) {
      fprintf(out, ",\n");
    }
    fprintf(out, "  [%zu] = { .type = %s, .alternative_number = %zu, ",
        i,
        alternative_types_char[alternative_type], i);
    const char *name = acr_alternative_get_object_to_swap_name(alternative);
    switch (alternative_type) {
      case acr_alternative_parameter:
      fprintf(out,
          ".value = { .alt.parameter.parameter_value = %zu"
          " , .name_to_swap = \"%s\""
          , acr_alternative_get_replacement_parameter(alternative),
          name);
      switch (options->type) {
          case acr_regular_build:
          case acr_perf_kernel_only:
          case acr_perf_compile_time_zero:
          case acr_perf_compile_time_zero_run:
            fprintf(out,
                " , .alt.parameter.parameter_id = %zu"
                " , .alt.parameter.function_matching_alternative ="
                " %s_acr_initial_%zu } "
                ,osl_strings_find(parameters, name), prefix, num_function++);
            break;
          case acr_static_kernel:
            fprintf(out,
                " , .alt.parameter.parameter_id = %zu"
                " } "
                , osl_strings_find(pragma_alternative_parameters, name));
            break;
        }
        break;
      case acr_alternative_function:
        fprintf(out,
            ".value = { .alt.function_to_swap = \"%s\""
            " , .name_to_swap = \"%s\" } ",
            acr_alternative_get_replacement_function(alternative),
            name);
        break;
      case acr_alternative_corner_computation:
      case acr_alternative_zero_computation:
      case acr_alternative_full_computation:
      case acr_alternative_unknown:
        break;
    }
    fprintf(out, "}");
  }
  fprintf(out, "\n};\n");
  osl_strings_free(pragma_alternative_parameters);
}

static void acr_print_get_alternetive_from_val(
    FILE *out,
    const char* prefix,
    size_t strategy_list_size,
    acr_option_list strategy_list,
    size_t *strategy_to_alternative_index) {

  fprintf(out, "static struct runtime_alternative *%s_alternative_fun[%u] = {\n",
      prefix, UCHAR_MAX);
  intmax_t max_current;
  intmax_t min_current;
  bool not_first_alternative = false;
  for (size_t i = 0; i < strategy_list_size; ++i) {
      acr_option current = acr_option_list_get_option(i, strategy_list);
      max_current = acr_strategy_get_max(current);
      min_current = acr_strategy_get_min(current);
      for (intmax_t j = min_current; j <= max_current; ++j) {
        if (not_first_alternative)
          fprintf(out, ",\n");
        else
          not_first_alternative = true;
        fprintf(out, "    [%zu] = &%s_alternatives[%zu]", j, prefix,
            strategy_to_alternative_index[i]);
      }
  }
  fprintf(out, "\n};\n");
  fprintf(out, "static inline struct runtime_alternative* %s_get_alternative_from_val(\n"
      "    unsigned char data) {\n"
      "  return %s_alternative_fun[data];\n"
      "}\n", prefix, prefix);
}

static void cloog_print_function_for_all_alternative_parameters(FILE *out,
    const char *prefix,
    size_t num_alternatives,
    const acr_option_list alternative_list,
    const size_t *strategy_to_alternative_index,
    const acr_compute_node node,
    const osl_scop_p scop) {

  fprintf(out, "\n");

  CloogState *state = cloog_state_malloc();
  CloogOptions *cloog_option = cloog_options_malloc(state);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->scop = scop;
  cloog_option->otl = 1;
  cloog_option->language = 0;

  const acr_option init =
    acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  osl_strings_p parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);

  size_t num_function = 0;
  for (size_t i = 0; i < num_alternatives; ++i) {
    acr_option alternative = acr_option_list_get_option(
        strategy_to_alternative_index[i], alternative_list);
    if (acr_alternative_get_type(alternative) == acr_alternative_parameter) {
      CloogInput *cloog_input = cloog_input_from_osl_scop(state,
          scop);
      isl_set *context = isl_set_from_cloog_domain(cloog_input->context);
      char *parameter_name =
        acr_alternative_get_object_to_swap_name(alternative);
      size_t parameter_position = osl_strings_find(parameters, parameter_name);
      intmax_t parameter_value =
        acr_alternative_get_replacement_parameter(alternative);
      isl_val *val =
        isl_val_int_from_si(isl_set_get_ctx(context), parameter_value);
      isl_constraint *constraint =
        isl_constraint_alloc_equality(
            isl_local_space_from_space(isl_set_get_space(context)));
      constraint = isl_constraint_set_constant_val(constraint, val);
      constraint = isl_constraint_set_coefficient_si(constraint, isl_dim_param,
          (int)parameter_position, -1);
      context = isl_set_add_constraint(context, constraint);
      cloog_input->context = cloog_domain_from_isl_set(context);

      fprintf(out, "static void %s_acr_initial_%zu", prefix, num_function++);
      acr_print_parameters(out, init);
      fprintf(out, " {\n");
      CloogProgram *cloog_program =
        cloog_program_alloc(cloog_input->context, cloog_input->ud, cloog_option);
      cloog_program = cloog_program_generate(cloog_program, cloog_option);
      cloog_program_pprint(out, cloog_program, cloog_option);
      cloog_program_free(cloog_program);
      free(cloog_input);
      fprintf(out, "}\n\n");
    }
  }

  cloog_state_free(state);
  cloog_option->openscop = 0;
  cloog_option->scop = NULL;
  cloog_options_free(cloog_option);
}

static bool print_alternative_preamble(
    acr_option_list *strategy_list,
    acr_option_list *alternative_list,
    size_t ** strategy_to_alternative_index,
    size_t *num_alternatives,
    size_t *num_strategy,
    const acr_compute_node node) {

  size_t size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  for (size_t i = 0; i < size_list; ++i) {
    acr_option current_option = acr_option_list_get_option(i, options);
    if (acr_option_get_type(current_option) == acr_type_strategy) {
      ++*num_strategy;
    }
    if (acr_option_get_type(current_option) == acr_type_alternative) {
      ++*num_alternatives;
    }
  }
  if (*num_strategy == 0) {
    fprintf(stderr,
        "[ACR] Warning: There is no strategies left for this node:\n");
    pprint_acr_compute_node(stderr, node, 0);
    return false;
  }
  if (*num_alternatives == 0) {
    fprintf(stderr,
        "[ACR] Warning: There is no alternatives left for this node:\n");
    pprint_acr_compute_node(stderr, node, 0);
    return false;
  }
  *strategy_list = acr_new_option_list(*num_strategy);
  *strategy_to_alternative_index =
    malloc(*num_strategy * sizeof(*strategy_to_alternative_index));
  *alternative_list = acr_new_option_list(*num_alternatives);

  acr_populate_strategy_and_alternative_list(
      node,
      *num_strategy,
      *strategy_list,
      *num_alternatives,
      *alternative_list,
      *strategy_to_alternative_index);

  return true;
}

static bool acr_print_static_alternative_tab(FILE *out,
    const acr_compute_node node,
    const osl_scop_p scop,
    const struct acr_build_options *options) {
  const char* prefix = acr_get_scop_prefix(node);
  size_t num_strategy = 0;
  size_t num_alternatives = 0;
  acr_option_list strategy_list;
  acr_option_list alternative_list;
  size_t *strategy_to_alternative_index;

  if (!print_alternative_preamble(&strategy_list, &alternative_list,
                                  &strategy_to_alternative_index,
                                  &num_alternatives, &num_strategy, node))
    return false;

  acr_print_acr_alternatives(out,
      prefix,
      num_alternatives,
      alternative_list,
      scop,
      options);

  free(strategy_list);
  free(strategy_to_alternative_index);
  free(alternative_list);

  return true;
}

static bool acr_print_acr_alternative_and_strategy_init(FILE* out,
    const acr_compute_node node,
    const osl_scop_p scop,
    const struct acr_build_options *options) {
  const char* prefix = acr_get_scop_prefix(node);
  size_t num_strategy = 0;
  size_t num_alternatives = 0;
  acr_option_list strategy_list;
  acr_option_list alternative_list;
  size_t *strategy_to_alternative_index;

  if (!print_alternative_preamble(&strategy_list, &alternative_list,
                                  &strategy_to_alternative_index,
                                  &num_alternatives, &num_strategy, node))
    return false;

  cloog_print_function_for_all_alternative_parameters(
      out, prefix,
      num_alternatives, alternative_list,
      strategy_to_alternative_index, node, scop);

  acr_print_acr_alternatives(out,
      prefix,
      num_alternatives,
      alternative_list,
      scop,
      options);

  acr_print_get_alternetive_from_val(out,
      prefix, num_strategy, strategy_list, strategy_to_alternative_index);

  free(strategy_list);
  free(strategy_to_alternative_index);
  free(alternative_list);

  return true;
}

static void acr_print_isl_lex_min_max_bound(FILE *out,
    bool upper_bound,
    isl_set *set,
    size_t dim_to_print,
    const osl_strings_p parameters) {
  const char max_char[] = "__acr__max__";
  const char min_char[] = "__acr__min__";
  const char *aggregator = upper_bound ? max_char : min_char;
  size_t num_dim = isl_set_n_dim(set);
  size_t num_param = isl_set_n_param(set);
  isl_set *only_dim_wanted = isl_set_copy(set);
  if (dim_to_print+1 < num_dim)
    only_dim_wanted = isl_set_project_out(only_dim_wanted, isl_dim_set,
        (unsigned int) (dim_to_print+1),
        (unsigned int) (num_dim - dim_to_print - 1));
  if (dim_to_print != 0)
    only_dim_wanted = isl_set_project_out(only_dim_wanted, isl_dim_set,
        0, (unsigned int) dim_to_print);
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
        dim_val = isl_val_add_ui(dim_val, 1);
        isl_printer_print_val(printer, dim_val);
        isl_printer_flush(printer);
        isl_val_free(dim_val);
        for (size_t k = 0; k < num_param; ++k) {
          if (isl_constraint_involves_dims(
                co, isl_dim_param, (unsigned int)k, 1)) {
            fprintf(out, " + %s*", parameters->string[k]);
            dim_val =
              isl_constraint_get_coefficient_val(
                  co, isl_dim_param, (int)k);
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
    const dimensions_upper_lower_bounds *bounds,
    uintmax_t tiling_size,
    const osl_scop_p scop) {
  size_t num_monitor_dims = 0;
  for (size_t i = 0; i < bounds->num_dimensions; ++i) {
    if (bounds->dimensions_type[i] == acr_dimension_type_bound_to_monitor)
      num_monitor_dims++;
  }
  osl_strings_p parameters = osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);

  fprintf(out,
      "  %s_runtime_data.monitor_dim_max =\n"
      "    malloc(%zu* sizeof(*%s_runtime_data.monitor_dim_max));\n"
      "#ifdef __acr__max__\n"
      "#undef __acr__max__\n"
      "#endif\n"
      "#define __acr__max__(a,b) (((a)>(b))?(a):(b))\n"
      "#ifdef __acr__min__\n"
      "#undef __acr__min__\n"
      "#endif\n"
      "#define __acr__min__(a,b) (((a)<(b))?(a):(b))\n",
      prefix, num_monitor_dims, prefix);

  size_t current_dim = 0;
  for (size_t i = 0; i < bounds->num_dimensions; ++i) {
    if (bounds->dimensions_type[i] == acr_dimension_type_bound_to_monitor) {
      fprintf(out, "  %s_runtime_data.monitor_dim_max[%zu] = ((",
          prefix, current_dim++);
      acr_print_isl_lex_min_max_bound(out,
          false, bounds->bound_lexmax, i, parameters);
      fprintf(out, ") %% %zu == 0) ? (", tiling_size);
      acr_print_isl_lex_min_max_bound(out,
          false, bounds->bound_lexmax, i, parameters);
      fprintf(out, ") / %zu : (", tiling_size);
      acr_print_isl_lex_min_max_bound(out,
          false, bounds->bound_lexmax, i, parameters);
      fprintf(out, ") / %zu + 1;\n", tiling_size);
    }
  }
  fprintf(out, "#undef __acr__max__\n");
  fprintf(out, "#undef __acr__min__\n");
}

static void acr_print_get_rid_of_parameters(
    FILE *out, const char *prefix, const osl_scop_p scop) {
  osl_strings_p parameters = osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);
  size_t num_param= osl_strings_size(parameters);
  for (size_t i = 0; i < num_param; ++i) {
    fprintf(out,
        "  acr_cloog_get_rid_of_parameter(&%s_runtime_data, %zu, %s);\n",
        prefix, i, parameters->string[i]);
  }
}

static void acr_print_static_get_rid_of_parameters(
    FILE *out,
    const osl_scop_p scop,
    const acr_compute_node node) {

  osl_strings_p pragma_alternative_parameters = osl_strings_malloc();
  size_t string_size = 0;
  const acr_option_list opt_list = acr_compute_node_get_option_list(node);
  const size_t opt_list_size = acr_compute_node_get_option_list_size(node);
  for (size_t i = 0; i < opt_list_size; ++i) {
    acr_option current_option =
      acr_option_list_get_option(i, opt_list);
    if (acr_option_get_type(current_option) == acr_type_alternative &&
        acr_alternative_get_type(current_option) == acr_alternative_parameter) {
      const char *parameter_to_swap =
        acr_alternative_get_object_to_swap_name(current_option);
      if (osl_strings_find(pragma_alternative_parameters, parameter_to_swap)
          == string_size) {
        osl_strings_add(pragma_alternative_parameters, parameter_to_swap);
        string_size += 1;
      }
    }
  }
  osl_strings_p parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);

  sort_osl_strings(pragma_alternative_parameters, parameters);
  size_t total_parameters = osl_strings_size(parameters);
  const char* prefix = acr_get_scop_prefix(node);
  size_t index = 0;
  for (size_t i = 0; i < total_parameters; ++i) {
    const char *current_parameter = parameters->string[i];
    if (osl_strings_find(pragma_alternative_parameters, current_parameter) == string_size) {
      fprintf(out, "    acr_cloog_get_rid_of_parameter_static("
        "&%s_static_runtime, %zu, %s);\n"
        , prefix, index, current_parameter);
    } else {
      index += 1;
    }
  }
  osl_strings_free(pragma_alternative_parameters);

}

static
char const*const acr_kernel_strategy_type_string[acr_runtime_kernel_unknown] =
  {
    [acr_runtime_kernel_simple]      = "acr_kernel_strategy_simple",
    [acr_runtime_kernel_versioning] = "acr_kernel_strategy_versioning",
    [acr_runtime_kernel_stencil]     = "acr_kernel_strategy_stencil",
  };

static char* acr_static_scan_code_corpse(const acr_option monitor) {
  char *retbuffer;
  size_t retbuffer_size;
  FILE *tmp_stream = open_memstream(&retbuffer, &retbuffer_size);
  const char *filter = acr_monitor_get_filter_name(monitor);
  acr_array_declaration *arr_decl = acr_monitor_get_array_declaration(monitor);
  const char *array_name = acr_array_decl_get_array_name(arr_decl);

  fprintf(tmp_stream,
      "  const unsigned char __acr_char_val = %s(%s", filter, array_name);
  size_t num_dims = acr_array_decl_get_num_dimensions(arr_decl);
  acr_array_dimensions_list adl = acr_array_decl_get_dimensions_list(arr_decl);
  for (size_t i = 0; i < num_dims; ++i) {
    char const*const identifier = acr_array_dimension_get_identifier(adl[i]);
    fprintf(tmp_stream, "[%s]", identifier);
  }
  fprintf(tmp_stream, ");");

  switch(acr_monitor_get_function(monitor)) {
    case acr_monitor_function_min:
      fprintf(tmp_stream,
          " __acr_tmp = __acr_tmp < __acr_char_val ? __acr_tmp : __acr_char_val;");
      break;
    case acr_monitor_function_max:
      fprintf(tmp_stream,
          " __acr_tmp = __acr_tmp > __acr_char_val ? __acr_tmp : __acr_char_val;");
      break;
    case acr_monitor_function_avg:
      fprintf(tmp_stream,
          " __acr_tmp += __acr_char_val; __acr_num_values += 1;");
      break;
    case acr_monitor_function_unknown:
      break;
  }
  fclose(tmp_stream);
  return retbuffer;
}

static const char** acr_get_monitor_id_list(const acr_option monitor) {
  acr_array_declaration *arr_decl = acr_monitor_get_array_declaration(monitor);
  size_t num_dims = acr_array_decl_get_num_dimensions(arr_decl);
  acr_array_dimensions_list adl = acr_array_decl_get_dimensions_list(arr_decl);
  char const**const identifiers_id = malloc((num_dims + 1) * sizeof(*identifiers_id));
  for (size_t i = 0; i < num_dims; ++i) {
    char const*const identifier = acr_array_dimension_get_identifier(adl[i]);
    identifiers_id[i] = identifier;
  }
  identifiers_id[num_dims] = NULL;
  return identifiers_id;
}

static void acr_print_static_monitor_ids(FILE *out,
    const acr_option monitor) {
  const char**const identifier_list = acr_get_monitor_id_list(monitor);
  fprintf(out, "  .iterators = (char const*const[]){\n");
  size_t identifier_num = 0;
  while (identifier_list[identifier_num] != NULL) {
    fprintf(out, "    [%zu] = \"%s\",\n", identifier_num,
        identifier_list[identifier_num]);
    identifier_num += 1;
  }
  fprintf(out, "    [%zu] = NULL,\n },\n", identifier_num);
  free(identifier_list);
}

#include <errno.h>
static void acr_print_static_function_parameters(FILE *out,
                                                 const acr_compute_node node) {
  const acr_option init = acr_compute_node_get_option_of_type(acr_type_init,
      node, 1);
  fprintf(out, "  .function_parameters = \"");
  acr_print_parameters(out, init);
  // Get rid of the ')' character
  // // GLIBC BUG
  /*fseek(out, 0L, SEEK_END);*/
  /*fseek(out, -1L, SEEK_CUR);*/
  if(fseek(out, ftell(out)-1, SEEK_SET)) {
    perror("fseek");
    exit(1);
  }
  fprintf(out, ", void const**const __acr_function_pointer, void const*const*const __acr_function_array)\",\n");
}
static bool acr_print_static_runtime_init(FILE* out,
    const osl_scop_p scop,
    const acr_compute_node node) {

  const char* prefix = acr_get_scop_prefix(node);
  size_t num_alternatives = 0;
  size_t size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  for (size_t i = 0; i < size_list; ++i) {
    acr_option current_option = acr_option_list_get_option(i, options);
    if (acr_option_get_type(current_option) == acr_type_alternative) {
      ++num_alternatives;
    }
  }
  size_t first_monitor_dimension, num_monitor_dims;
  acr_openscop_get_monitoring_position_and_num(
      node, scop, &first_monitor_dimension, &num_monitor_dims);

  if (num_monitor_dims == 0) {
    fprintf(stderr,
        "[ACR] error: Cannot find the first monitoring dimension"
        " inside the kernel\n");
    return false;
  }
  const acr_option grid =
    acr_compute_node_get_option_of_type(acr_type_grid, node, 1);
  if (!grid) {
    fprintf(stderr, "[ACR] error: The current node has no grid info\n");
    return false;
  }

  char *reduction_function;
  acr_option monitor =
    acr_compute_node_get_option_of_type(acr_type_monitor, node, 1);
  enum acr_monitor_processing_funtion proc_fun_type =
    acr_monitor_get_function(monitor);
  char *scan_code = acr_static_scan_code_corpse(monitor);
  switch (proc_fun_type) {
    case acr_monitor_function_min:
      reduction_function = "acr_reduction_min";
      break;
    case acr_monitor_function_max:
      reduction_function = "acr_reduction_max";
      break;
    case acr_monitor_function_avg:
      reduction_function = "acr_reduction_avg";
      break;
    default:
      reduction_function = NULL;
      break;
  }

  fprintf(out,
      "static struct acr_runtime_data_static %s_static_runtime = {\n"
      "  .num_alternatives = %zu,\n"
      "  .alternatives = %s_alternatives,\n"
      "  .first_monitor_dimension = %zu,\n"
      "  .num_monitor_dimensions = %zu,\n"
      "  .grid_size = %zu,\n"
      "  .reduction_function = %s,\n"
      "  .scan_corpse = \"%s\",\n"
      , prefix, num_alternatives, prefix, first_monitor_dimension,
      num_monitor_dims, acr_grid_get_grid_size(grid), reduction_function,
      scan_code);
  acr_print_static_monitor_ids(out, monitor);
  acr_print_static_function_parameters(out, node);
  fprintf(out, "};\n");
  free(scan_code);
  return true;
}

static void acr_print_acr_runtime_init(FILE* out,
    const acr_compute_node node,
    const dimensions_upper_lower_bounds_all_statements *dims,
    const dimensions_upper_lower_bounds *bounds,
    const osl_scop_p scop,
    struct acr_build_options const*build_options) {
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  const char* prefix = acr_init_get_function_name(init);
  size_t num_alternatives = 0;

  size_t size_list = acr_compute_node_get_option_list_size(node);
  acr_option_list options = acr_compute_node_get_option_list(node);
  for (size_t i = 0; i < size_list; ++i) {
    acr_option current_option = acr_option_list_get_option(i, options);
    if (acr_option_get_type(current_option) == acr_type_alternative) {
      ++num_alternatives;
    }
  }
  size_t num_monitor_dims = 0;
  for (size_t i = 0; i < bounds->num_dimensions; ++i) {
    if (bounds->dimensions_type[i] == acr_dimension_type_bound_to_monitor)
      num_monitor_dims++;
  }

  acr_option grid = acr_compute_node_get_option_of_type(acr_type_grid, node, 1);
  fprintf(out, "static void %s_monitoring_function(unsigned char*);\n", prefix);

  fprintf(out,
      "#ifdef ACR_STATS_ENABLED\n"
      "static struct acr_runtime_stats %s_runtime_stats;\n"
      "#endif // ACR_STATS_ENABLED\n"
      , prefix);

  fprintf(out,
      "static struct acr_runtime_kernel_info %s_runtime_kernel_info;\n"
      "static struct acr_runtime_data %s_runtime_data = {\n"
      "  .kernel_info = &%s_runtime_kernel_info,\n"
      "  .kernel_strategy_type = %s,\n"
      "  .kernel_prefix = \"%s\",\n"
      "  .num_alternatives = %zu,\n"
      "  .alternatives = %s_alternatives,\n"
      "  .num_monitor_dims = %zu,\n"
      "  .grid_size = %zu,\n"
      "  .num_statements = %zu,\n"
      "  .dimensions_per_statements = (unsigned int [%zu]) {\n",
      prefix, prefix,
      prefix, acr_kernel_strategy_type_string[build_options->kernel_version],
      prefix, num_alternatives, prefix,
      num_monitor_dims, acr_grid_get_grid_size(grid),
      dims->num_statements, dims->num_statements);
  for (size_t i = 0; i < dims->num_statements; ++i) {
    const dimensions_upper_lower_bounds *current_bound =
      dims->statements_bounds[i];
    if (i > 0)
      fprintf(out, ",\n");
    fprintf(out, "    [%zu] = %zu",
        i, current_bound->num_dimensions);
  }
  fprintf(out,
      "\n  },\n"
      "#ifdef ACR_STATS_ENABLED\n"
      "  .acr_stats = &%s_runtime_stats,\n"
      "#endif // ACR_STATS_ENABLED\n"
      "  .alternative_from_val = %s_get_alternative_from_val,\n"
      "  .monitoring_function = %s_monitoring_function,\n"
      "  .alternative_still_usable = 0,\n"
      "  .monitor_thread_continue = true,\n"
      "  .function_prototype = \"",
      prefix, prefix, prefix);
  acr_print_parameters(out, init);
  fprintf(out, "\",\n");
  fprintf(out,
      "  .statement_dimension_types = (enum acr_dimension_type* [%zu]) {\n",
      dims->num_statements);
  for (size_t i = 0; i < dims->num_statements; ++i) {
    const dimensions_upper_lower_bounds *current_bound =
      dims->statements_bounds[i];
    if (i > 0)
      fprintf(out, ",\n");
    fprintf(out, "    [%zu] = (enum acr_dimension_type[%zu]) {",
        i, current_bound->num_dimensions);
    for (size_t j = 0; j < current_bound->num_dimensions; ++j) {
      if (j > 0)
        fprintf(out, ", ");
      switch (current_bound->dimensions_type[j]) {
        case acr_dimension_type_bound_to_alternative:
          fprintf(out, "acr_dimension_type_bound_to_alternative");
          break;
        case acr_dimension_type_bound_to_monitor:
          fprintf(out, "acr_dimension_type_bound_to_monitor");
          break;
        case acr_dimension_type_free_dim:
          fprintf(out, "acr_dimension_type_free_dim");
          break;
      }
    }
    fprintf(out, "}");
  }
  acr_option_list list = acr_compute_node_get_option_list(node);
  size_t list_size = acr_compute_node_get_option_list_size(node);
  bool has_alternative_parameter = false;
  for (size_t i = 0; !has_alternative_parameter && i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, list);
    if (acr_option_get_type(current_option) == acr_type_alternative &&
        acr_alternative_get_type(current_option) == acr_alternative_parameter) {
      has_alternative_parameter = true;
    }
  }

  fprintf(out, "\n  },\n");

  if (has_alternative_parameter) {
      fprintf(out, "  .original_function = %s_acr_initial_0,\n",
          prefix);
  } else {
      fprintf(out, "  .original_function = %s_acr_initial,\n",
          prefix);
  }

  fprintf(out, "};\n");

  switch (build_options->type) {
    case acr_regular_build:
      break;
    case acr_perf_compile_time_zero_run:
      fprintf(out, "#include \"acr_perf_%s\"\n", prefix);
    case acr_perf_kernel_only:
    case acr_perf_compile_time_zero:
      fprintf(out,
          "static struct acr_runtime_perf %s_runtime_perf = {\n"
          "  .rdata = &%s_runtime_data,\n"
          "  .list_head = NULL,\n"
          "  .compilation_list = NULL,\n"
          "  .list_size = 0,\n"
          , prefix, prefix);
      break;
    case acr_static_kernel:
      break;
  }
  switch (build_options->type) {
    case acr_regular_build:
      break;
    case acr_perf_kernel_only:
      fprintf(out,
          "  .type = acr_runtime_perf_kernel_only,\n"
          "};\n");
      break;
    case acr_perf_compile_time_zero:
      fprintf(out,
          "  .type = acr_runtime_perf_compilation_time_zero,\n"
          "};\n");
      break;
    case acr_perf_compile_time_zero_run:
      fprintf(out,
          "  .type = acr_runtime_perf_compilation_time_zero_run,\n"
          "};\n");
      break;
    case acr_static_kernel:
      break;
  }

  fprintf(out, "static void %s_acr_runtime_init", prefix);
  acr_print_parameters(out, init);
  fprintf(out, " {\n");

  uintmax_t tiling_size =
    acr_grid_get_grid_size(acr_compute_node_get_option_of_type(acr_type_grid, node, 1));
  acr_print_monitor_max_dims(out, prefix, bounds, tiling_size, scop);

  switch (build_options->type) {
    case acr_perf_compile_time_zero_run:
      fprintf(out,
          "  acr_recover_perf_%s(&%s_runtime_perf);\n",
          prefix, prefix);
    case acr_regular_build:
    fprintf(out,
        "  init_acr_runtime_data_thread_specific(&%s_runtime_data);\n",
        prefix);
      break;
    case acr_perf_kernel_only:
    case acr_perf_compile_time_zero:
    case acr_static_kernel:
      break;
  }
  fprintf(out,
      "  init_acr_runtime_data(\n"
      "      &%s_runtime_data,\n"
      "      %s_acr_scop,\n"
      "      %s_acr_scop_size);\n",
      prefix, prefix, prefix);

  acr_print_get_rid_of_parameters(out, prefix, scop);

  fprintf(out, "  acr_get_current_time(&%s_runtime_data.kernel_info->step_temp_time);\n",
      prefix);

  // Call function and change pointer to initial function
  if (has_alternative_parameter) {
    fprintf(out, "  %s = %s_acr_initial_0;\n  ",
        prefix, prefix);
  } else {
    fprintf(out, "  %s = %s_acr_initial;\n  ",
        prefix, prefix);
  }
  acr_print_init_function_call(out, init, build_options);
  switch (build_options->type) {
    case acr_regular_build:
    fprintf(out,
        "  pthread_create(&%s_runtime_data.monitor_thread, NULL,\n"
        "    acr_verification_and_coordinator_function, &%s_runtime_data);\n",
        prefix, prefix);
      break;
    case acr_perf_compile_time_zero_run:
    fprintf(out,
        "  %s_runtime_perf.rdata = &%s_runtime_data;\n"
        "  pthread_create(&%s_runtime_data.monitor_thread, NULL,\n"
        "    acr_runtime_perf_compile_time_zero, &%s_runtime_perf);\n",
        prefix, prefix, prefix, prefix);
      break;
    case acr_perf_kernel_only:
    case acr_perf_compile_time_zero:
    case acr_static_kernel:
      break;
  }
  fprintf(out, "}\n\n");

}

static void acr_print_init_function_declaration(FILE* kernel_file, FILE* out,
    const acr_option init,
    size_t kernel_start, size_t kernel_end) {

  fprintf(out, "static void %s_acr_runtime_init",
      acr_init_get_function_name(init));
  acr_print_parameters(out, init);
  fprintf(out, ";\n");
  fprintf(out, "static void %s_acr_initial", acr_init_get_function_name(init));
  acr_print_parameters(out, init);
  fprintf(out, " {\n");
  fseek(kernel_file, (long) kernel_start, SEEK_SET);
  acr_copy_from_file_to_file(kernel_file, out, kernel_start, kernel_end);
  fprintf(out, "\n}\n\n");

  fprintf(out, "static void (*%s)", acr_init_get_function_name(init));
  acr_print_parameters(out, init);
  fprintf(out, " = %s_acr_runtime_init;\n", acr_init_get_function_name(init));
}

bool acr_print_node_initialization(FILE* in, FILE* out,
    const acr_compute_node node,
    size_t kernel_start, size_t kernel_end,
    struct acr_build_options const* options) {

  acr_option init;
  char *prefix = acr_get_scop_prefix(node);
  switch (options->type) {
    case acr_regular_build:
    case acr_perf_kernel_only:
    case acr_perf_compile_time_zero:
    case acr_perf_compile_time_zero_run:
      fprintf(out, "#define %s_acr &%s_runtime_data\n", prefix, prefix);
      init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
      if (init == NULL) {
        fprintf(stderr, "Error no initialization in current node\n");
        pprint_acr_compute_node(stderr, node, 0);
        return false;
      }
      acr_print_init_function_declaration(in, out, init,
          kernel_start, kernel_end);
      break;
    case acr_static_kernel:
      fprintf(out, "#define %s_acr &%s_static_runtime\n", prefix, prefix);
      break;
  }

  return true;

}

static void acr_print_static_function_call(FILE* out,
    const acr_compute_node node,
    const struct acr_build_options *build_options) {
  const char* prefix = acr_get_scop_prefix(node);
  fprintf(out,
      "  const size_t __acr_total_size = %s_static_runtime.num_fun_per_alt;\n"
      "  for (size_t __acr_iterator = 0; __acr_iterator < __acr_total_size; __acr_iterator += 1) {\n"
      "    void (*__acr_function_call)"
      , prefix);
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  acr_print_parameters(out, init);
  if(fseek(out, ftell(out)-3, SEEK_SET)) {
    perror("fseek");
    exit(1);
  }
  fprintf(out,
      ", void *const*const __acr_function_pointer,"
      " void const*const __acr_function_array, size_t __acr_num_of_fun_per_alt)"
      " = %s_static_runtime.functions[__acr_iterator];\n"
      "    __acr_function_call"
      , prefix);
  acr_print_init_function_call(out, init, build_options);
  if(fseek(out, ftell(out)-3, SEEK_SET)) {
    perror("fseek");
    exit(1);
  }
  fprintf(out,
      ", &%s_static_runtime.functions[__acr_iterator]"
      ",%s_static_runtime.all_functions, %s_static_runtime.num_fun_per_alt);\n  }\n"
      ,prefix, prefix, prefix);
}

void acr_print_static_function(FILE* out,
    const acr_compute_node node,
    const osl_scop_p scop,
    const struct acr_build_options *build_options) {
  const char* prefix = acr_get_scop_prefix(node);
  fprintf(out,
      "  static int __acr_is_initialized = 0;\n"
      "  if (!__acr_is_initialized) {\n"
      "    init_acr_static_data(&%s_static_runtime, %s_acr_scop, %s_acr_scop_size);\n"
      , prefix, prefix, prefix);
  acr_print_static_get_rid_of_parameters(out, scop, node);
  fprintf(out,
      "    acr_static_data_init_grid(&%s_static_runtime);"
      "    __acr_is_initialized = 1;\n"
      "  }\n"
      , prefix);
  acr_print_static_function_call(out, node, build_options);
}

void acr_print_node_init_function_call(FILE* out,
    const acr_compute_node node, const struct acr_build_options *b_options) {
  const char* prefix = acr_get_scop_prefix(node);
  fprintf(out,
      "#ifdef ACR_STATS_ENABLED\n"
      "  acr_time t0;\n"
      "  acr_get_current_time(&t0);\n"
      "#endif\n");
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  acr_print_init_function_call(out, init, b_options);

  fprintf(out, "  acr_time sim_step_t2;\n"
      "  acr_get_current_time(&sim_step_t2);\n"
      "  double current_sim_step_time = acr_difftime(%s_runtime_data.kernel_info->step_temp_time, sim_step_t2);\n"
      "  %s_runtime_data.kernel_info->sim_step_time = %s_runtime_data.kernel_info->sim_step_time * 0.8 +"
      "  current_sim_step_time * 0.2;\n"
      "  %s_runtime_data.kernel_info->step_temp_time = sim_step_t2;\n"
      "  %s_runtime_data.kernel_info->num_calls += 1;\n"
      , prefix, prefix, prefix, prefix, prefix);

  fprintf(out,
      "#ifdef ACR_STATS_ENABLED\n"
      "  acr_time t1;\n"
      "  acr_get_current_time(&t1);\n"
      "  pthread_spin_lock(&%s_runtime_data.alternative_lock);\n"
      "  %s_runtime_data.acr_stats->sim_stats.total_time += acr_difftime(t0, t1);\n"
      "  %s_runtime_data.acr_stats->sim_stats.num_simmulation_step += 1;\n"
      "#else\n"
      "  pthread_spin_lock(&%s_runtime_data.alternative_lock);\n"
      "#endif\n"
      "  if (%s_runtime_data.alternative_still_usable) {\n"
      "    if (%s_runtime_data.alternative_function) {\n"
      "      %s = (void (*)",
      prefix, prefix, prefix, prefix, prefix, prefix, prefix);
  acr_print_parameters(out, init);

  acr_option_list list = acr_compute_node_get_option_list(node);
  size_t list_size = acr_compute_node_get_option_list_size(node);
  bool has_alternative_parameter = false;
  for (size_t i = 0; !has_alternative_parameter && i < list_size; ++i) {
    acr_option current_option = acr_option_list_get_option(i, list);
    if (acr_option_get_type(current_option) == acr_type_alternative &&
        acr_alternative_get_type(current_option) == acr_alternative_parameter) {
      has_alternative_parameter = true;
    }
  }

  if (has_alternative_parameter) {
    fprintf(out,
        ") %s_runtime_data.alternative_function;\n"
        "      %s_runtime_data.alternative_function = NULL;\n"
        "    }\n"
        /*"    fprintf(stderr, \"[Compute] Using new\\n\");\n"*/
        "  } else {\n"
        "    %s = %s_acr_initial_0;\n"
        /*"    fprintf(stderr, \"[Compute] Using initial\\n\");\n"*/
        "  }\n"
        "  pthread_spin_unlock(&%s_runtime_data.alternative_lock);\n",
        prefix, prefix, prefix, prefix, prefix);
  } else {
    fprintf(out,
        ") %s_runtime_data.alternative_function;\n"
        "      %s_runtime_data.alternative_function = NULL;\n"
        "    }\n"
        /*"    fprintf(stderr, \"[Compute] Using new\\n\");\n"*/
        "  } else {\n"
        "    %s = %s_acr_initial;\n"
        /*"    fprintf(stderr, \"[Compute] Using initial\\n\");\n"*/
        "  }\n"
        "  pthread_spin_unlock(&%s_runtime_data.alternative_lock);\n",
        prefix, prefix, prefix, prefix, prefix);
  }
}

void acr_print_node_init_function_call_for_max_perf_run(FILE *out,
    const acr_compute_node node, const struct acr_build_options *build_options) {
  char* prefix = acr_get_scop_prefix(node);
  acr_option init = acr_compute_node_get_option_of_type(acr_type_init, node, 1);
  acr_print_init_function_call(out, init, build_options);
  fprintf(out, "  %s = (void (*)", prefix);
  acr_print_parameters(out, init);
  fprintf(out, ") acr_runntime_perf_end_step(&%s_runtime_perf);", prefix);
}

static void acr_print_init_function_call(FILE* out, const acr_option init,
    const struct acr_build_options *build_options) {
  switch (build_options->type) {
    case acr_regular_build:
    case acr_perf_kernel_only:
    case acr_perf_compile_time_zero:
    case acr_perf_compile_time_zero_run:
      fprintf(out, "%s", acr_init_get_function_name(init));
      break;
    case acr_static_kernel:
      break;
  }
  fprintf(out, "(");
  size_t num_parameters = acr_init_get_num_parameters(init);
  acr_parameter_declaration_list declaration_list =
    acr_init_get_parameter_list(init);
  if (num_parameters != 1 ||
      (num_parameters == 1 && strcmp(
        acr_parameter_declaration_get_parameter_name(declaration_list, 0),
        "void") != 0))
  for (size_t i = 0; i < num_parameters; ++i) {
    if (i != 0)
      fprintf(out, ", ");
    fprintf(out, "%s",
        acr_parameter_declaration_get_parameter_name(declaration_list,i));
  }
  fprintf(out, ");\n");
}

static void acr_print_scop_in_file(FILE* output,
    const char* scop_prefix, osl_scop_p scop) {
  char* buffer = NULL;
  osl_generic_remove(&scop->extension, OSL_URI_COORDINATES);
  size_t sizebuffer;
  acr_print_scop_to_buffer(scop, &buffer, &sizebuffer);
  fprintf(output, "static size_t %s_acr_scop_size = %zu;\n", scop_prefix,
      sizebuffer);
  fprintf(output, "static char %s_acr_scop[] = \n\"", scop_prefix);
  size_t current_position = 0;
  while (buffer[current_position] != '\0') {
    if (buffer[current_position] == '\n') {
      fprintf(output, "\\n\"\n\"");
    } else {
      if(buffer[current_position] == '"')
        fprintf(output, "\\\"");
      else
        fprintf(output, "%c", buffer[current_position]);
    }
    ++current_position;
  }
  fprintf(output, "\";\n\n");
  free(buffer);
}

static void acr_print_destroy(FILE* out, const char *prefix,
    struct acr_build_options const*build_options) {
  switch (build_options->type) {
    case acr_regular_build:
      fprintf(out,
          "  free_acr_runtime_data_thread_specific(&%s_runtime_data);\n"
          "  free_acr_runtime_data(&%s_runtime_data);\n",
          prefix, prefix);
      break;
    case acr_perf_kernel_only:
    case acr_perf_compile_time_zero:
      fprintf(out,
          "  acr_runtime_perf_clean(&%s_runtime_perf);\n"
          ,prefix);
      break;
    case acr_perf_compile_time_zero_run:
      fprintf(out,
          "  free_acr_runtime_data_thread_specific(&%s_runtime_data);\n"
          "  acr_runtime_clean_time_zero_run(&%s_runtime_perf);\n",
          prefix, prefix);
      break;
    case acr_static_kernel:
      fprintf(out,
          "  free_acr_static_data(&%s_static_runtime);\n", prefix);
      break;
  }
  switch (build_options->type) {
    case acr_regular_build:
    case acr_perf_compile_time_zero_run:
      fprintf(out,
          "#ifdef ACR_STATS_ENABLED\n"
          "  acr_print_stats(stderr, %s_runtime_data.kernel_prefix,"
          "  &%s_runtime_data.acr_stats->sim_stats,\n  &%s_runtime_data.acr_stats->thread_stats,\n"
          "  %s_runtime_data.num_codegen_threads,\n"
          "  %s_runtime_data.num_compile_threads,\n"
          "  %s_runtime_data.kernel_info->sim_step_time);\n"
          "#endif\n"
          , prefix, prefix, prefix, prefix, prefix, prefix);
      break;
    default:
      break;
  }
}

static void print_monitor_array_access(
    FILE *out,
    const char *array_name,
    size_t num_monitor_dims) {
  fprintf(out, "%s[", array_name);
  for(size_t current_dim = 0; current_dim < num_monitor_dims; ++current_dim) {
    if (current_dim)
      fprintf(out, " + ");
    fprintf(out, "(c%zu", current_dim * 2 + 2);
    for(size_t j = current_dim + 1; j < num_monitor_dims; ++j) {
      fprintf(out, " * __acr_monitor_dim_%zu", j);
    }
    fprintf(out, ")");
  }
  fprintf(out, "]");
}

static void print_array_access_function(
    FILE *out,
    const char *array_name,
    const char *filter_name,
    acr_array_declaration *arr_decl) {

  size_t num_dims = acr_array_decl_get_num_dimensions(arr_decl);
  if (filter_name)
    fprintf(out, "%s(%s", filter_name, array_name);
  else
    fprintf(out, "%s", array_name);
  acr_array_dimensions_list dim_list = acr_array_decl_get_dimensions_list(arr_decl);
  for(size_t i = 0; i < num_dims; ++i) {
    const char *dim_name = acr_array_dimension_get_identifier(dim_list[i]);
      fprintf(out, "[%s]", dim_name);
  }
  if (filter_name)
    fprintf(out, ")");
}

struct monitoring_statement{
  char *main_statement;
  char *init_statement;
  char *end_statement;
};

static struct monitoring_statement get_main_statements(
    enum acr_monitor_processing_funtion processing_function,
    const char *monitor_array_name,
    const acr_option monitor,
    size_t num_monitor_dims) {
  acr_array_declaration *arr_decl =  acr_monitor_get_array_declaration(monitor);
  const char* array_name = acr_array_decl_get_array_name(arr_decl);
  const char* filter = acr_monitor_get_filter_name(monitor);

  char *buffer1, *buffer2, *buffer3;
  size_t size_buffer1, size_buffer2, size_buffer3;
  FILE *temp_buffer1, *temp_buffer2, *temp_buffer3;
  temp_buffer1 = open_memstream(&buffer1, &size_buffer1);
  temp_buffer2 = open_memstream(&buffer2, &size_buffer2);

  struct monitoring_statement monstate;
  switch(processing_function) {

    case acr_monitor_function_max:
    case acr_monitor_function_min:
      print_monitor_array_access(temp_buffer1, monitor_array_name, num_monitor_dims);
      fprintf(temp_buffer1, " = ");
      print_monitor_array_access(temp_buffer1, monitor_array_name, num_monitor_dims);
      if (processing_function == acr_monitor_function_max) {
        fprintf(temp_buffer1, " > ");
      } else {
        fprintf(temp_buffer1, " < ");
      }
      print_array_access_function(temp_buffer1, array_name, filter, arr_decl);
      fprintf(temp_buffer1, " ? ");
      print_monitor_array_access(temp_buffer1, monitor_array_name, num_monitor_dims);
      fprintf(temp_buffer1, " : ");
      print_array_access_function(temp_buffer1, array_name, filter, arr_decl);
      fprintf(temp_buffer1, ";");
      print_monitor_array_access(temp_buffer2, monitor_array_name, num_monitor_dims);
      fprintf(temp_buffer2, " = ");
      print_array_access_function(temp_buffer2, array_name, filter, arr_decl);
      fprintf(temp_buffer2, ";");
      break;

    case acr_monitor_function_avg:
      temp_buffer3 = open_memstream(&buffer3, &size_buffer3);
      fprintf(temp_buffer1, "__acr_tmp_avg += ");
      print_array_access_function(temp_buffer1, array_name, filter, arr_decl);
      fprintf(temp_buffer1, "; __acr_num_values += 1;");
      fprintf(temp_buffer2, "__acr_tmp_avg = 0; "
                            "__acr_num_values = 0;");
      print_monitor_array_access(temp_buffer3, monitor_array_name, num_monitor_dims);
      fprintf(temp_buffer3, " = __acr_tmp_avg / __acr_num_values;");
      fclose(temp_buffer3);
      monstate.end_statement = buffer3;
      break;
    case acr_monitor_function_unknown:
      break;
  }
  fclose(temp_buffer1);
  monstate.main_statement = buffer1;
  fclose(temp_buffer2);
  monstate.init_statement = buffer2;

  return monstate;
}

static void acr_set_dimensions_to_equality(
    size_t dim_num,
    size_t dim2_num,
    size_t multiplier,
    isl_map **map) {
  isl_val *constval = isl_val_int_from_ui(isl_map_get_ctx(*map), multiplier);
  isl_space *myspace = isl_map_get_space(*map);
  isl_constraint *c =
    isl_constraint_alloc_equality(isl_local_space_from_space(myspace));
  c = isl_constraint_set_coefficient_val(c, isl_dim_out, (int)dim_num, constval);
  c = isl_constraint_set_coefficient_si(c, isl_dim_out, (int)dim2_num, -1);
  *map = isl_map_add_constraint(*map, c);
}

static void acr_set_dimension_to_value(
    size_t dim_num,
    size_t value,
    isl_map **map) {
  isl_val *constval = isl_val_int_from_ui(isl_map_get_ctx(*map), value);
  isl_space *myspace = isl_map_get_space(*map);
  isl_constraint *c =
    isl_constraint_alloc_equality(isl_local_space_from_space(myspace));
  c = isl_constraint_set_constant_val(c, constval);
  c = isl_constraint_set_coefficient_si(c, isl_dim_out, (int) dim_num, -1);
  *map =
    isl_map_drop_constraints_involving_dims(*map, isl_dim_out, (unsigned int) dim_num, 1);
  *map = isl_map_add_constraint(*map, c);
}

static void acr_modify_main_statement_for_test_type(
    size_t tiling_size,
    size_t num_monitor_dims,
    enum acr_monitor_processing_funtion processing_function,
    struct monitoring_statement mon_statements,
    osl_scop_p scop,
    CloogUnionDomain **ud) {
  osl_body_p initial_body =
    osl_generic_lookup(scop->statement->extension, OSL_URI_BODY);
  if (!initial_body) {
    fprintf(stderr, "No body in original scop\n");
    exit(1);
  }
  osl_statement_p new_statement = osl_statement_malloc();
  osl_strings_p corpse = osl_strings_malloc();
  osl_strings_add(corpse, mon_statements.init_statement);
  osl_body_p body = osl_body_malloc();
  body->iterators = osl_strings_clone(initial_body->iterators);
  body->expression = corpse;
  osl_generic_add(&new_statement->extension, osl_generic_shell(body, osl_body_interface()));
  osl_statement_add(&scop->statement, new_statement);

  isl_set *copied_domain =
    isl_set_copy((isl_set*) (*ud)->domain->domain);
  isl_map *copied_scattering =
    isl_map_copy((isl_map*) (*ud)->domain->scattering);
  for (size_t i = 0; i < num_monitor_dims; ++i) {
    acr_set_dimensions_to_equality(
        i*2+1,
        i*2+1+num_monitor_dims*2,
        tiling_size, &copied_scattering);
  }
  acr_set_dimension_to_value(num_monitor_dims * 2, 0, &copied_scattering);
  acr_set_dimension_to_value(num_monitor_dims * 2, 1,
      (isl_map**) &(*ud)->domain->scattering);
  *ud = cloog_union_domain_add_domain(*ud, NULL, (CloogDomain*) copied_domain, (CloogScattering*) copied_scattering, NULL);

  if (processing_function == acr_monitor_function_avg) {
    copied_domain =
      isl_set_copy((isl_set*) (*ud)->domain->domain);
    copied_scattering =
      isl_map_copy((isl_map*) (*ud)->domain->scattering);
    for (size_t i = 0; i < num_monitor_dims; ++i) {
      acr_set_dimensions_to_equality(
          i*2+1,
          i*2+1+num_monitor_dims*2,
          tiling_size, &copied_scattering);
    }
    acr_set_dimension_to_value(num_monitor_dims * 2, 2, &copied_scattering);
    *ud = cloog_union_domain_add_domain(*ud, NULL, (CloogDomain*) copied_domain, (CloogScattering*) copied_scattering, NULL);
    new_statement = osl_statement_malloc();
    corpse = osl_strings_malloc();
    osl_strings_add(corpse, mon_statements.end_statement);
    body = osl_body_malloc();
    body->iterators = osl_strings_clone(initial_body->iterators);
    body->expression = corpse;
    osl_generic_add(&new_statement->extension, osl_generic_shell(body, osl_body_interface()));
    osl_statement_add(&scop->statement, new_statement);
  }
}

static bool acr_print_scanning_function(FILE* out, const acr_compute_node node,
    osl_scop_p scop,
    const dimensions_upper_lower_bounds_all_statements *dims,
    struct acr_build_options const*build_options) {
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
  uintmax_t grid_size = acr_grid_get_grid_size(grid);
  const char *prefix = acr_get_scop_prefix(node);
  dimensions_upper_lower_bounds *bound_used;
  osl_scop_p new_scop = acr_openscop_gen_monitor_loop(monitor, scop,
      grid_size, dims, &bound_used, prefix);
  if (new_scop == NULL) {
    fprintf(stderr, "It is not possible to find monitor data boundaries\n");
    return false;
  }
  acr_print_acr_runtime_init(out, node, dims, bound_used, scop,
      build_options);

  fprintf(out,
      "static void %s_monitoring_function(unsigned char* monitor_result) {\n",
      prefix);
  switch (acr_monitor_get_function(monitor)) {
    case acr_monitor_function_min:
    case acr_monitor_function_max:
      break;
    case acr_monitor_function_avg:
      fprintf(out, "size_t __acr_tmp_avg, __acr_num_values;\n");
      break;
    case acr_monitor_function_unknown:
      break;
  }

  osl_scop_free(new_scop);

  CloogState *cloog_state = cloog_state_malloc();
  CloogInput *cloog_input = cloog_input_from_osl_scop(cloog_state, scop);

  size_t first_monitor_dimension, num_monitor_dims;
  acr_openscop_get_monitoring_position_and_num(
      node, scop, &first_monitor_dimension, &num_monitor_dims);

  acr_runtime_apply_tiling(grid_size, first_monitor_dimension, num_monitor_dims, cloog_input->ud);

  const char **const identifiers_id = acr_get_monitor_id_list(monitor);

  for (size_t i = 0; i < num_monitor_dims; ++i) {
    fprintf(out,
        "size_t __acr_monitor_dim_%zu = %s_runtime_data.monitor_dim_max[%zu];\n"
        , i, prefix, i);
  }

  struct monitoring_statement mon_statements =
    get_main_statements(
        acr_monitor_get_function(monitor),
        "monitor_result",
        monitor,
        num_monitor_dims);

  CloogUnionDomain *new_ud;
  CloogDomain *new_context;
  acr_runtime_apply_reduction_function(
      cloog_input->context, cloog_input->ud,
      first_monitor_dimension, num_monitor_dims,
      &new_ud, &new_context, &new_scop,
      mon_statements.main_statement, identifiers_id, true);

  free(identifiers_id);
  acr_modify_main_statement_for_test_type(
      grid_size,
      num_monitor_dims,
      acr_monitor_get_function(monitor),
      mon_statements,
      new_scop,
      &new_ud);

  new_ud->n_name[CLOOG_PARAM] = cloog_input->ud->n_name[CLOOG_PARAM];
  new_ud->name[CLOOG_PARAM] = cloog_input->ud->name[CLOOG_PARAM];
  cloog_input->ud->n_name[CLOOG_PARAM] = 0;
  cloog_input->ud->name[CLOOG_PARAM] = NULL;

  CloogOptions *cloog_option = cloog_options_malloc(cloog_state);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->scop = new_scop;
  cloog_option->otl = 1;
  cloog_option->language = 0;
  CloogProgram *cloog_program = cloog_program_alloc(new_context,
      new_ud, cloog_option);
  cloog_program = cloog_program_generate(cloog_program, cloog_option);

  cloog_program_pprint(out, cloog_program, cloog_option);
  fprintf(out, "}\n");

  cloog_union_domain_free(cloog_input->ud);
  cloog_domain_free(cloog_input->context);
  cloog_program_free(cloog_program);
  cloog_state_free(cloog_state);
  cloog_options_free(cloog_option);
  free(cloog_input);
  free(mon_statements.main_statement);
  free(mon_statements.init_statement);
  if (acr_monitor_get_function(monitor) == acr_monitor_function_avg)
    free(mon_statements.end_statement);
  return true;
}

static void acr_delete_alternative_parameters_for_parameter_not_present_in_scop(
    acr_compute_node node,
    const osl_scop_p scop) {

  osl_strings_p parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);
  size_t total_parameters = osl_strings_size(parameters);

  for (size_t i = 0; i < acr_compute_node_get_option_list_size(node); ++i) {
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
          i -= 1;
        }
      }
    }
  }
}

static void acr_add_corresponding_deferred_destroy_to_do_in_future(
    const acr_compute_node node,
    const acr_option_list general_opt,
    size_t num_general_opt,
    acr_option **option_in_future,
    size_t *num_option_in_future,
    size_t current_position) {
  for (size_t i = 0; i < num_general_opt; ++i) {
    acr_option current_option = acr_option_list_get_option(i, general_opt);
    size_t pragma_position = acr_option_get_pragma_position(current_option);
    if (pragma_position < current_position)
      continue;
    if (acr_option_get_type(current_option) == acr_type_deferred_destroy) {
      char *deferred_id = acr_deferred_destroy_get_ref_init_name(current_option);
      acr_option init =
        acr_compute_node_get_option_of_type(acr_type_init, node, 1);
      char *fun_name = acr_init_get_function_name(init);
      if (strcmp(fun_name, deferred_id) == 0) {
        acr_option destroy =
          acr_compute_node_get_option_of_type(acr_type_destroy, node, 1);
        size_t init_pos = acr_option_get_pragma_position(init);
        size_t destroy_pos = acr_option_get_pragma_position(destroy);
        size_t def_destroy_pos = acr_option_get_pragma_position(current_option);
        if (def_destroy_pos < destroy_pos || destroy_pos < init_pos) {
          fprintf(stderr,
              "[ACR] Warning: Deferred destroy construct must"
              " appear *after* the *init* and *destroy* they refer to.\n"
              "               The following deferred destroy will be ignored:\n");
          pprint_acr_option(stderr, current_option, 0);
        } else {
          *num_option_in_future += 1;
          *option_in_future = realloc(*option_in_future,
              *num_option_in_future * sizeof(**option_in_future));
          option_in_future[0][*num_option_in_future-1] = current_option;
        }
      }
    }
  }
}

static size_t check_and_apply_option_in_future(
    FILE *input, FILE *output, const acr_compute_node all_pragmas,
    size_t next_position,
    size_t current_position,
    acr_option const*const option_in_future,
    size_t num_option_in_future,
    struct acr_build_options const*build_options) {
  for (size_t i = 0; i < num_option_in_future; ++i) {
    size_t option_position =
      acr_option_get_pragma_position(option_in_future[i]);
    if (option_position > next_position)
      break;
    else {
      if (option_position > current_position) {
        current_position = acr_copy_from_file_avoiding_pragmas(input, output,
            current_position, option_position, all_pragmas);
        const char* prefix = acr_deferred_destroy_get_ref_init_name(option_in_future[i]);
        acr_print_destroy(output, prefix, build_options);
      }
    }
  }
  return current_position;
}
static bool acr_deferred_destroy_for_node(
    const acr_compute_node node,
    acr_option const*const option_in_future,
    size_t num_option_in_future) {
  const char *fun_name =
    acr_init_get_function_name(
        acr_compute_node_get_option_of_type(acr_type_init, node, 1));
  for (size_t i = 0; i < num_option_in_future; ++i) {
    if (acr_option_get_type(option_in_future[i]) == acr_type_deferred_destroy) {
      if (strcmp(fun_name, acr_deferred_destroy_get_ref_init_name(option_in_future[i])) == 0)
        return true;
    }
  }
  return false;
}

static void simplify_osl_for_printing(osl_scop_p scop) {
  osl_generic_remove(&scop->extension, OSL_URI_SCATNAMES);
  osl_generic_remove(&scop->extension, OSL_URI_ARRAYS);
  osl_statement_p statement = scop->statement;
  while (statement) {
    osl_relation_list_free(statement->access);
    statement->access = NULL;
    statement = statement->next;
  }
}

static void acr_generate_code_static(
    const char* filename,
    const struct acr_build_options *build_options,
    FILE *current_file,
    const acr_compute_node all_options,
    const acr_compute_node_list node_list,
    const acr_option_list general_options,
    size_t num_general_options) {

  acr_option *option_to_apply_in_the_future = NULL;
  size_t num_option_to_apply_in_future = 0;
  char* new_file_name = acr_new_file_name(filename);
  FILE* new_file = fopen(new_file_name, "w");
  if (!new_file) {
    perror("fopen");
    return;
  }
  acr_generate_preamble(new_file, filename);
  const size_t list_size = acr_compute_node_list_get_size(node_list);
  size_t position_in_input = 0;
  fseek(current_file, (long)position_in_input, SEEK_SET);
  for (size_t i = 0; i < list_size; ++i) {
    acr_compute_node node = acr_compute_node_list_get_node(i, node_list);
    osl_scop_p scop = acr_extract_scop_in_compute_node(
        node, current_file, filename);
    if (scop) {
      if (scop->next) {
        osl_scop_free(scop->next);
        scop->next = NULL;
      }
      simplify_osl_for_printing(scop);
      acr_delete_alternative_parameters_for_parameter_not_present_in_scop(
          node,
          scop);
      while(acr_simplify_compute_node(node));

      acr_add_corresponding_deferred_destroy_to_do_in_future(node,
          general_options, num_general_options,  &option_to_apply_in_the_future,
          &num_option_to_apply_in_future, position_in_input);

      char *buffer;
      size_t size_buffer;
      FILE* temp_buffer = open_memstream(&buffer, &size_buffer);
      const size_t scop_start_position = position_in_input;
      size_t kernel_start, kernel_end;
      acr_scop_coord_to_acr_coord(current_file, scop, node,
          &kernel_start, &kernel_end);

      fseek(current_file, (long)position_in_input, SEEK_SET);
      position_in_input = check_and_apply_option_in_future(
          current_file, temp_buffer, all_options,
          acr_position_of_init_in_node(node), position_in_input,
          option_to_apply_in_the_future, num_option_to_apply_in_future,
          build_options);
      position_in_input = acr_copy_from_file_avoiding_pragmas(
          current_file, temp_buffer, position_in_input,
          acr_position_of_init_in_node(node), all_options);

      const char* prefix = acr_get_scop_prefix(node);
      acr_print_scop_in_file(temp_buffer, prefix, scop);

      if (!acr_print_static_alternative_tab(
            temp_buffer, node, scop, build_options)) {
        fclose(temp_buffer);
        free(buffer);
        position_in_input = scop_start_position;
        osl_scop_free(scop);
        continue;
      }

      if (!acr_print_node_initialization(current_file, temp_buffer, node,
            kernel_start, kernel_end, build_options)) {
        fclose(temp_buffer);
        free(buffer);
        position_in_input = scop_start_position;
        osl_scop_free(scop);
        continue;
      }

      if(!acr_print_static_runtime_init(temp_buffer, scop, node)) {
        fclose(temp_buffer);
        free(buffer);
        position_in_input = scop_start_position;
        osl_scop_free(scop);
        continue;
      }

      fseek(current_file, (long)position_in_input, SEEK_SET);
      position_in_input = acr_copy_from_file_avoiding_pragmas(
          current_file,
          temp_buffer,
          position_in_input, kernel_start,
          all_options);

      acr_print_static_function(temp_buffer, node, scop, build_options);

      position_in_input = kernel_end;
      fseek(current_file, (long)position_in_input, SEEK_SET);

      acr_option destroy =
        acr_compute_node_get_option_of_type(acr_type_destroy, node, 1);
      size_t destroy_pos = acr_destroy_get_pragma_position(destroy);
      position_in_input = acr_copy_from_file_avoiding_pragmas(
          current_file,
          temp_buffer,
          position_in_input, destroy_pos,
          all_options);
      if (!acr_deferred_destroy_for_node(node, option_to_apply_in_the_future,
            num_option_to_apply_in_future)) {
        acr_print_destroy(temp_buffer, prefix, build_options);
      }
      fclose(temp_buffer);
      fprintf(new_file, "%s", buffer);
      free(buffer);
    }
  }
  fseek(current_file, 0, SEEK_END);
  size_t end_file = (size_t) ftell(current_file);
  fseek(current_file, (long)position_in_input, SEEK_SET);
  position_in_input = check_and_apply_option_in_future(
      current_file, new_file, all_options,
      end_file, position_in_input,
      option_to_apply_in_the_future, num_option_to_apply_in_future,
      build_options);
  position_in_input = acr_copy_from_file_avoiding_pragmas(
      current_file, new_file, position_in_input,
      end_file, all_options);
  free(new_file_name);
  fclose(new_file);
  free(option_to_apply_in_the_future);
}

static void acr_generate_code_dynamic(
    const char* filename,
    const struct acr_build_options *build_options,
    FILE *current_file,
    const acr_compute_node all_options,
    acr_compute_node_list node_list,
    const acr_option_list general_options,
    size_t num_general_options) {

  acr_option *option_to_apply_in_the_future = NULL;
  size_t num_option_to_apply_in_future = 0;
  char* new_file_name = acr_new_file_name(filename);
  FILE* new_file = fopen(new_file_name, "w");
  if (!new_file) {
    perror("fopen");
    return;
  }
  acr_generate_preamble(new_file, filename);
  const size_t list_size = acr_compute_node_list_get_size(node_list);
  size_t position_in_input = 0;
  fseek(current_file, (long)position_in_input, SEEK_SET);
  for (size_t i = 0; i < list_size; ++i) {
    acr_compute_node node = acr_compute_node_list_get_node(i, node_list);
    osl_scop_p scop = acr_extract_scop_in_compute_node(
        node, current_file, filename);
    if (scop) {
      if (scop->next) {
        osl_scop_free(scop->next);
        scop->next = NULL;
      }
      simplify_osl_for_printing(scop);
      acr_delete_alternative_parameters_for_parameter_not_present_in_scop(
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

      acr_add_corresponding_deferred_destroy_to_do_in_future(node,
          general_options, num_general_options,  &option_to_apply_in_the_future,
          &num_option_to_apply_in_future, position_in_input);

      char *buffer;
      size_t size_buffer;
      FILE* temp_buffer = open_memstream(&buffer, &size_buffer);
      const size_t scop_start_position = position_in_input;
      const char* prefix = acr_get_scop_prefix(node);
      size_t kernel_start, kernel_end;
      acr_scop_coord_to_acr_coord(current_file, scop, node,
          &kernel_start, &kernel_end);

      fseek(current_file, (long)position_in_input, SEEK_SET);
      position_in_input = check_and_apply_option_in_future(
          current_file, temp_buffer, all_options,
          acr_position_of_init_in_node(node), position_in_input,
          option_to_apply_in_the_future, num_option_to_apply_in_future,
          build_options);
      position_in_input = acr_copy_from_file_avoiding_pragmas(
          current_file, temp_buffer, position_in_input,
          acr_position_of_init_in_node(node), all_options);

      acr_print_scop_in_file(temp_buffer, prefix, scop);

      if (!acr_print_node_initialization(current_file, temp_buffer, node,
            kernel_start, kernel_end, build_options)) {
        fclose(temp_buffer);
        free(buffer);
        position_in_input = scop_start_position;
        osl_scop_free(scop);
        continue;
      }

      if(!acr_print_acr_alternative_and_strategy_init(
            temp_buffer, node, scop, build_options)) {
        fclose(temp_buffer);
        free(buffer);
        position_in_input = scop_start_position;
        osl_scop_free(scop);
        acr_osl_free_dimension_upper_lower_bounds_all(bounds_all);
        continue;
      }

      if (!acr_print_scanning_function(temp_buffer, node, scop, bounds_all,
            build_options)) {
        fclose(temp_buffer);
        free(buffer);
        position_in_input = scop_start_position;
        osl_scop_free(scop);
        acr_osl_free_dimension_upper_lower_bounds_all(bounds_all);
        continue;
      }
      fseek(current_file, (long)position_in_input, SEEK_SET);
      position_in_input = acr_copy_from_file_avoiding_pragmas(
          current_file,
          temp_buffer,
          position_in_input, kernel_start,
          all_options);

      switch (build_options->type) {
        case acr_regular_build:
        case acr_perf_compile_time_zero_run:
          acr_print_node_init_function_call(temp_buffer, node, build_options);
          break;
        case acr_perf_kernel_only:
        case acr_perf_compile_time_zero:
          acr_print_node_init_function_call_for_max_perf_run(temp_buffer, node, build_options);
          break;
        case acr_static_kernel:
          break;
      }
      position_in_input = kernel_end;
      fseek(current_file, (long)position_in_input, SEEK_SET);
      osl_scop_free(scop);

      acr_option destroy =
        acr_compute_node_get_option_of_type(acr_type_destroy, node, 1);
      size_t destroy_pos = acr_destroy_get_pragma_position(destroy);
      position_in_input = acr_copy_from_file_avoiding_pragmas(
          current_file,
          temp_buffer,
          position_in_input, destroy_pos,
          all_options);
      if (!acr_deferred_destroy_for_node(node, option_to_apply_in_the_future,
            num_option_to_apply_in_future)) {
        acr_print_destroy(temp_buffer, prefix, build_options);
      }
      fclose(temp_buffer);
      fprintf(new_file, "%s", buffer);
      free(buffer);
      acr_osl_free_dimension_upper_lower_bounds_all(bounds_all);
    }
  }
  fseek(current_file, 0, SEEK_END);
  size_t end_file = (size_t) ftell(current_file);
  fseek(current_file, (long)position_in_input, SEEK_SET);
  position_in_input = check_and_apply_option_in_future(
      current_file, new_file, all_options,
      end_file, position_in_input,
      option_to_apply_in_the_future, num_option_to_apply_in_future,
      build_options);
  position_in_input = acr_copy_from_file_avoiding_pragmas(
      current_file, new_file, position_in_input,
      end_file, all_options);
  free(new_file_name);
  fclose(new_file);
  free(option_to_apply_in_the_future);
}

void acr_generate_code(const char* filename,
    struct acr_build_options *build_options) {
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

  size_t num_general_options;
  acr_option_list general_options = acr_get_general_option_list(all_options,
      &num_general_options);


  if (node_list) {
    switch (build_options->type) {
      case acr_static_kernel:
        acr_generate_code_static(filename, build_options,
            current_file, all_options, node_list,
            general_options, num_general_options);
        break;
      default:
        acr_generate_code_dynamic(filename, build_options,
            current_file, all_options, node_list,
            general_options, num_general_options);
        break;
    }
  }

  acr_free_option_list(general_options, num_general_options);
  acr_free_compute_node_list(node_list);
  acr_free_compute_node(all_options);
  fclose(current_file);
}

void acr_generate_preamble(FILE* file, const char* filename) {
  fprintf(file,
      " /*\n"
      "   File automatically generated by ACR\n"
      "   Defunct input file: %s\n"
      "   Do not read the following line.\n"
      "   If you read this know that you don't deserve candy!\n"
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
    size_t num_general_options;
    acr_option_list general_options = acr_get_general_option_list(compute_node,
        &num_general_options);

    fprintf(out, "##### GENERAL OPTIONS #####\n");
    pprint_acr_option_list(out, general_options, num_general_options, 0);

    acr_compute_node_list node_list =
      acr_new_compute_node_list_split_node(compute_node);
    if (node_list) {
      for(size_t j = 0; j < node_list->list_size; ++j) {
        acr_compute_node node = acr_compute_node_list_get_node(j, node_list);
        osl_scop_p scop = acr_extract_scop_in_compute_node(
            node, current_file, filename);
        if (scop) {
          if (scop->next) {
            osl_scop_free(scop->next);
            scop->next = NULL;
          }
          acr_delete_alternative_parameters_for_parameter_not_present_in_scop(
              node,
              scop);
          while(acr_simplify_compute_node(node));
          fprintf(out, "##### ACR options for node %zu: #####\n", j);
          pprint_acr_compute_node(out, node, 0);
          fprintf(out, "##### Scop for node %zu: #####\n", j);
          osl_scop_print(out, scop);
          osl_scop_free(scop);
        } else {
          fprintf(out, "##### No scop found for node %zu: #####", j);
          pprint_acr_compute_node(out, node, 0);
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

  size_t start, end;
  acr_get_start_and_stop_for_clan(node, &start, &end);

  osl_scop_p scop = clan_scop_extract_delimited(input_file,
      &clan_options, start, end);
  free(clan_options.name);
  return scop;
}

void acr_get_start_and_stop_for_clan(const acr_compute_node node,
    size_t* start, size_t *stop) {

  *start = 0;
  *stop = 0;

  size_t option_list_size = acr_compute_node_get_option_list_size(node);
  const acr_option_list option_list = acr_compute_node_get_option_list(node);

  for (size_t i = 0; i < option_list_size; ++i) {
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
      case acr_type_deferred_destroy:
      case acr_type_unknown:
        break;
    }
  }
}

void acr_scop_coord_to_acr_coord(
    FILE* kernel_file,
    const osl_scop_p scop,
    const acr_compute_node compute_node,
    size_t  *start,
    size_t *end) {
  acr_get_start_and_stop_for_clan(compute_node, start, end);
  osl_coordinates_p osl_coord = osl_generic_lookup(scop->extension,
      OSL_URI_COORDINATES);
  if (osl_coord == NULL)
    return;
  fseek(kernel_file, (long)*start, SEEK_SET);
  size_t line_num = 1;
  char c;
  size_t current_position = *start;
  while (line_num < (size_t) osl_coord->line_start) {
    if(fscanf(kernel_file, "%c", &c) == EOF)
      break;
    ++current_position;
    if (c == '\n')
      ++line_num;
  }
  if (osl_coord->column_start > 0) {
    current_position += (size_t)osl_coord->column_start - 1;
    fseek(kernel_file, (long)osl_coord->column_start - 1, SEEK_CUR);
  }
  *start = current_position;
  while (line_num < (size_t) osl_coord->line_end) {
    if(fscanf(kernel_file, "%c", &c) == EOF)
      break;
    ++current_position;
    if (c == '\n')
      ++line_num;
  }
  if (osl_coord->column_end > 0)
    current_position += (size_t)osl_coord->column_end;
  *end = current_position;
}
