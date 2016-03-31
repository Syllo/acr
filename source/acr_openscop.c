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

#include "acr/acr_openscop.h"

#include <isl/constraint.h>
#include <isl/set.h>
#include <osl/body.h>
#include <osl/extensions/scatnames.h>
#include <osl/macros.h>
#include <stdio.h>

#include "acr/print.h"

void acr_print_scop_to_buffer(osl_scop_p scop, char** buffer,
    size_t* size_buffer) {
  FILE* output_char = open_memstream(buffer, size_buffer);
  osl_scop_print(output_char, scop);
  fclose(output_char);
}

static void osl_set_scattering_base(
    unsigned long num_iterators,
    osl_relation_p scattering) {
  // update beta
  for (unsigned int i = 0; i < num_iterators; ++i) {
    unsigned long iterator_column = 2 + num_iterators * 2 + i;
    unsigned long scatter_row = i * 2 + 1;
    unsigned long scatter_column = 2 + i * 2;
    unsigned long beta_row = i * 2;
    unsigned long beta_column = 1 + i * 2;
    osl_int_decrement(scattering->precision, &scattering->m[beta_row][beta_column],
        scattering->m[beta_row][beta_column]);
    osl_int_decrement(scattering->precision,
        &scattering->m[scatter_row][scatter_column],
        scattering->m[scatter_row][scatter_column]);
    osl_int_increment(scattering->precision,
        &scattering->m[scatter_row][iterator_column],
        scattering->m[scatter_row][iterator_column]);
  }
  osl_int_decrement(scattering->precision,
      &scattering->m[num_iterators*2][num_iterators*2+1],
      scattering->m[num_iterators*2][num_iterators*2+1]);
}

void osl_relation_swap_column(osl_relation_p relation,
    int pos1,
    int pos2) {
  if (relation == NULL || pos1 > relation->nb_columns || pos2 > relation->nb_columns)
    return;

  osl_int_p temp = osl_int_malloc(relation->precision);

  for (int i = 0; i < relation->nb_rows; ++i) {
    osl_int_assign(relation->precision,
        temp, relation->m[i][pos1]);
    osl_int_assign(relation->precision,
        &relation->m[i][pos1], relation->m[i][pos2]);
    osl_int_assign(relation->precision,
       &relation->m[i][pos2], *temp);
  }

  osl_int_free(relation->precision, temp);
}

static void acr_osl_apply_tiling(
    unsigned long num_iterators,
    osl_relation_p scattering,
    unsigned long tiling_size) {
  osl_int_p temp_val = osl_int_malloc(scattering->precision);
  int last_row = scattering->nb_rows - 1;
  for (unsigned long i = 0; i < num_iterators; ++i) {
    int where_to_add_two_column = 2 + i * 4;
    osl_relation_insert_blank_column(scattering, where_to_add_two_column);

    // Set beta to zero
    osl_relation_insert_blank_row(scattering, -1);
    last_row += 1;
    osl_int_decrement(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column],
        scattering->m[last_row][where_to_add_two_column]);

    osl_relation_insert_blank_column(scattering, where_to_add_two_column);
    // Set new scatter to do strip-mine
    osl_relation_insert_blank_row(scattering, -1);
    last_row += 1;
    osl_int_set_si(scattering->precision, temp_val, tiling_size);
    osl_int_assign(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column], *temp_val);
    osl_int_assign(scattering->precision,
        &scattering->m[last_row][scattering->nb_columns-1],
        *temp_val);
    osl_int_decrement(scattering->precision,
        &scattering->m[last_row][scattering->nb_columns-1],
        scattering->m[last_row][scattering->nb_columns-1]);
    osl_int_increment(scattering->precision,
        &scattering->m[last_row][0],
        scattering->m[last_row][0]);
    osl_int_decrement(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column + 2],
        scattering->m[last_row][where_to_add_two_column + 2]);

    osl_relation_insert_blank_row(scattering, -1);
    last_row += 1;
    osl_int_oppose(scattering->precision,
        temp_val, *temp_val);
    osl_int_assign(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column], *temp_val);
    osl_int_increment(scattering->precision,
        &scattering->m[last_row][0],
        scattering->m[last_row][0]);
    osl_int_increment(scattering->precision,
        &scattering->m[last_row][where_to_add_two_column + 2],
        scattering->m[last_row][where_to_add_two_column + 2]);

    scattering->nb_output_dims += 2;

    for (int j = i; j > 0; --j) {
      osl_relation_swap_column(scattering, where_to_add_two_column, where_to_add_two_column - 2);
      where_to_add_two_column -= 2;
    }
  }
  osl_int_free(scattering->precision, temp_val);
}

static osl_relation_p acr_openscop_new_scattering_relation(
    unsigned long num_iterators, unsigned long num_parameters) {
  osl_relation_p scattering = osl_relation_malloc(
      num_iterators*2+1,
      3 + num_parameters + num_iterators * 3);
  osl_relation_set_type(scattering, OSL_TYPE_SCATTERING);
  osl_relation_set_attributes(scattering, num_iterators*2+1, num_iterators, 0, num_parameters);
  osl_set_scattering_base(num_iterators, scattering);
  return scattering;
}

static inline void _acr_openscop_add_missing_identifiers(
    unsigned long* size_string,
    osl_strings_p identifiers,
    const acr_array_dimension dimension) {
  if (osl_strings_find(identifiers, dimension->identifier)
      == *size_string) {
    osl_strings_add(identifiers, dimension->identifier);
    *size_string += 1;
  }
}

osl_strings_p acr_openscop_get_monitor_identifiers(const acr_option monitor) {
  size_t string_size = 0;
  osl_strings_p identifiers = osl_strings_malloc();
  acr_array_declaration* array_decl =
    acr_monitor_get_array_declaration(monitor);
  acr_array_dimensions_list dim_list = array_decl->array_dimensions_list;
  for (unsigned long i = 0; i < array_decl->num_dimensions; ++i) {
    _acr_openscop_add_missing_identifiers(&string_size, identifiers, dim_list[i]);
  }
  return identifiers;
}

unsigned long acr_monitor_num_dimensions(const acr_option monitor) {
  return monitor->options.monitor.data_monitored.num_dimensions;
}

bool acr_osl_check_if_identifiers_are_not_parameters(const osl_scop_p scop,
    osl_strings_p identifiers) {
  size_t num_identifiers = osl_strings_size(identifiers);
  osl_statement_p statements = scop->statement;
  while (statements) {
    osl_body_p body = osl_generic_lookup(statements->extension, OSL_URI_BODY);
    size_t num_iterators = osl_strings_size(body->iterators);
    for (size_t i = 0; i < num_identifiers; ++i) {
      if (osl_strings_find(body->iterators, identifiers->string[i]) < num_iterators) {
        return false;
      }
    }
    statements = statements->next;
  }
  return true;
}

static void acr_print_array_access_monitor_dim_n(
    FILE *out,
    unsigned long num_dimensions,
    const unsigned long *monitor_dim,
    unsigned long dimmension) {
  fprintf(out, "(c%lu", monitor_dim[dimmension]*2);
  for (unsigned long i = dimmension+1; i < num_dimensions; ++i) {
    fprintf(out, " * a_runtime_data.monitor_dim_max[%lu]", i);
  }
  fprintf(out, ")");
}

static inline void print_monitor_result_access(
    FILE* out,
    unsigned long num_dimensions,
    const unsigned long *monitor_dim) {
  fprintf(out, "monitor_result");
  fprintf(out, "[");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    if (i > 0)
      fprintf(out, " + ");
    acr_print_array_access_monitor_dim_n(out, num_dimensions,
        monitor_dim, i);
  }
  fprintf(out, "]");
}

static void acr_openscop_scan_min_max_op(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    bool max,
    const unsigned long *monitor_dim,
    osl_statement_p statement) {
  osl_body_p body = osl_body_malloc();
  unsigned int num_iterators = statement->domain->nb_output_dims;
  osl_strings_p iterators = osl_strings_generate("i", num_iterators);
  body->iterators = iterators;
  char* statement_string;
  size_t size_string;
  FILE* tempbuffer = open_memstream(&statement_string, &size_string);
  acr_array_declaration *decl = acr_monitor_get_array_declaration(monitor);
  unsigned long num_dimensions = acr_array_decl_get_num_dimensions(decl);
#ifndef ACR_DEBUG
  print_monitor_result_access(tempbuffer,
      num_dimensions, monitor_dim);
  fprintf(tempbuffer, " = ");
  print_monitor_result_access(tempbuffer,
      num_dimensions, monitor_dim);
  if (max) {
    fprintf(tempbuffer, " < ");
  }
  else {
    fprintf(tempbuffer, " > ");
  }
  if (filter_function) {
    fprintf(tempbuffer, "%s(", filter_function);
  }
  fprintf(tempbuffer, "%s", acr_array_decl_get_array_name(decl));
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[i%lu]", monitor_dim[i]);
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, " ? ");
  if (filter_function) {
    fprintf(tempbuffer, "%s(", filter_function);
  }
  fprintf(tempbuffer, "%s", acr_array_decl_get_array_name(decl));
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[i%lu]", monitor_dim[i]);
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, " : ");
  print_monitor_result_access(tempbuffer,
      num_dimensions, monitor_dim);
  fprintf(tempbuffer, ";");
#else
  (void) filter_function;
  (void) max;
  fprintf(tempbuffer, "fprintf(stderr, \"compare ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "= ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "\\n\"");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", c%lu", monitor_dim[i]*2);
  }
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", i%lu", monitor_dim[i]);
  }
  fprintf(tempbuffer, ");");
#endif
  fclose(tempbuffer);
  body->expression = osl_strings_encapsulate(statement_string);
  osl_generic_add(&statement->extension,
      osl_generic_shell(body, osl_body_interface()));
  osl_relation_p scattering = statement->scattering;
  osl_relation_insert_blank_row(scattering, -1);
  int row_position = scattering->nb_rows - 1;
  osl_int_decrement(scattering->precision,
      &scattering->m[row_position][scattering->nb_columns-1],
      scattering->m[row_position][scattering->nb_columns-1]);
  osl_int_increment(scattering->precision,
      &scattering->m[row_position][0],
      scattering->m[row_position][0]);
  osl_int_t temp;
  osl_int_init_set_si(scattering->precision, &temp, grid_size);
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    unsigned long initial_iterator_pos = 2 + 2*num_dimensions + i*2 ;
    unsigned long initial_scattering_pos = 2 + i*2;
    osl_int_increment(scattering->precision,
        &scattering->m[row_position][initial_iterator_pos],
        scattering->m[row_position][initial_iterator_pos]);
    osl_int_sub(scattering->precision,
        &scattering->m[row_position][initial_scattering_pos],
        scattering->m[row_position][initial_scattering_pos],
        temp);
  }
  osl_int_clear(scattering->precision, &temp);
}

static void acr_openscop_scan_init(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    enum acr_monitor_processing_funtion process_fun,
    const unsigned long *monitor_dim,
    osl_statement_p statement) {
  osl_body_p body = osl_body_malloc();
  unsigned int num_iterators = statement->domain->nb_output_dims;
  osl_strings_p iterators = osl_strings_generate("i", num_iterators);
  body->iterators = iterators;
  char* statement_string;
  size_t size_string;
  FILE* tempbuffer = open_memstream(&statement_string, &size_string);
  acr_array_declaration *decl = acr_monitor_get_array_declaration(monitor);
  unsigned long num_dimensions = acr_array_decl_get_num_dimensions(decl);
#ifndef ACR_DEBUG
  switch (process_fun) {
    case acr_monitor_function_min:
    case acr_monitor_function_max:
      print_monitor_result_access(tempbuffer,
          num_dimensions, monitor_dim);
      break;
    case acr_monitor_function_avg:
      fprintf(tempbuffer, "temp_avg");;
      break;
    case acr_monitor_function_unknown:
      break;
  }
  fprintf(tempbuffer, " = ");
  if (filter_function) {
    fprintf(tempbuffer, "%s(", filter_function);
  }
  fprintf(tempbuffer, "%s", acr_array_decl_get_array_name(decl));
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[i%lu]", monitor_dim[i]);
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, ";");
#else
  (void) filter_function;
  fprintf(tempbuffer, "fprintf(stderr, \"Scan init ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "= ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "\\n\"");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", c%lu", monitor_dim[i]*2);
  }
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", i%lu", monitor_dim[i]);
  }
  fprintf(tempbuffer, ");");
#endif
  if (process_fun == acr_monitor_function_avg) {
    fprintf(tempbuffer, " num_value = 1;");
  }
  fclose(tempbuffer);
  body->expression = osl_strings_encapsulate(statement_string);
  osl_generic_add(&statement->extension,
      osl_generic_shell(body, osl_body_interface()));
  osl_relation_p scattering = statement->scattering;
  osl_relation_insert_blank_row(scattering, -1);
  int row_position = scattering->nb_rows - 1;
  osl_int_t temp;
  osl_int_init_set_si(scattering->precision, &temp, grid_size);
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    unsigned long initial_iterator_pos = 2 + 2*num_dimensions + i*2 ;
    unsigned long initial_scattering_pos = 2 + i*2;
    osl_int_increment(scattering->precision,
        &scattering->m[row_position][initial_iterator_pos],
        scattering->m[row_position][initial_iterator_pos]);
    osl_int_sub(scattering->precision,
        &scattering->m[row_position][initial_scattering_pos],
        scattering->m[row_position][initial_scattering_pos],
        temp);
  }
  osl_int_clear(scattering->precision, &temp);
}

void acr_openscop_set_tiled_to_do_min_max(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    bool max,
    const unsigned long *monitor_dim,
    osl_scop_p scop) {
  osl_statement_p init, inf_or_sup;
  init = scop->statement;
  inf_or_sup = osl_statement_clone(init);
  acr_openscop_scan_init(monitor, filter_function, grid_size,
      acr_monitor_function_max,
      monitor_dim, init);
  acr_openscop_scan_min_max_op(monitor, filter_function, grid_size,
      max, monitor_dim, inf_or_sup);
  scop->statement->next = inf_or_sup;
}

static void acr_openscop_scan_avg_add(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    const unsigned long *monitor_dim,
    osl_statement_p statement) {
  osl_body_p body = osl_body_malloc();
  unsigned int num_iterators = statement->domain->nb_output_dims;
  osl_strings_p iterators = osl_strings_generate("i", num_iterators);
  body->iterators = iterators;
  char* statement_string;
  size_t size_string;
  FILE* tempbuffer = open_memstream(&statement_string, &size_string);
  acr_array_declaration *decl = acr_monitor_get_array_declaration(monitor);
  unsigned long num_dimensions = acr_array_decl_get_num_dimensions(decl);
#ifndef ACR_DEBUG
  fprintf(tempbuffer, "temp_avg += ");
  if (filter_function) {
    fprintf(tempbuffer, "%s(", filter_function);
  }
  fprintf(tempbuffer, "%s", acr_array_decl_get_array_name(decl));
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[i%lu]", monitor_dim[i]);
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, ";");
#else
  (void) filter_function;
  fprintf(tempbuffer, "fprintf(stderr, \"Avg add tempval += ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "\\n\"");
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", i%lu", monitor_dim[i]);
  }
  fprintf(tempbuffer, ");");
#endif
  fprintf(tempbuffer, " num_value += 1;");
  fclose(tempbuffer);
  body->expression = osl_strings_encapsulate(statement_string);
  osl_generic_add(&statement->extension,
      osl_generic_shell(body, osl_body_interface()));
  osl_relation_p scattering = statement->scattering;
  osl_relation_insert_blank_row(scattering, -1);
  int row_position = scattering->nb_rows - 1;
  osl_int_decrement(scattering->precision,
      &scattering->m[row_position][scattering->nb_columns-1],
      scattering->m[row_position][scattering->nb_columns-1]);
  osl_int_increment(scattering->precision,
      &scattering->m[row_position][0],
      scattering->m[row_position][0]);
  osl_int_t temp;
  osl_int_init_set_si(scattering->precision, &temp, grid_size);
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    unsigned long initial_iterator_pos = 2 + 2*num_dimensions + i*2 ;
    unsigned long initial_scattering_pos = 2 + i*2;
    osl_int_increment(scattering->precision,
        &scattering->m[row_position][initial_iterator_pos],
        scattering->m[row_position][initial_iterator_pos]);
    osl_int_sub(scattering->precision,
        &scattering->m[row_position][initial_scattering_pos],
        scattering->m[row_position][initial_scattering_pos],
        temp);
  }
  osl_int_clear(scattering->precision, &temp);
}

static void acr_openscop_scan_avg_div(
    const acr_option monitor,
    unsigned long grid_size,
    const unsigned long *monitor_dim,
    osl_statement_p statement) {
  osl_body_p body = osl_body_malloc();
  unsigned int num_iterators = statement->domain->nb_output_dims;
  osl_strings_p iterators = osl_strings_generate("i", num_iterators);
  body->iterators = iterators;
  char* statement_string;
  size_t size_string;
  FILE* tempbuffer = open_memstream(&statement_string, &size_string);
  acr_array_declaration *decl = acr_monitor_get_array_declaration(monitor);
  unsigned long num_dimensions = acr_array_decl_get_num_dimensions(decl);
#ifndef ACR_DEBUG
  print_monitor_result_access(tempbuffer,
      num_dimensions, monitor_dim);
  fprintf(tempbuffer, " = temp_avg / num_value;");
#else
  fprintf(tempbuffer, "fprintf(stderr, \"Avg div ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "by %%zu\\n\"");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", c%lu", monitor_dim[i]*2);
  }
  fprintf(tempbuffer, ", num_value);");
#endif
  fclose(tempbuffer);
  body->expression = osl_strings_encapsulate(statement_string);
  osl_generic_add(&statement->extension,
      osl_generic_shell(body, osl_body_interface()));
  osl_relation_p scattering = statement->scattering;
  osl_relation_insert_blank_row(scattering, -1);
  int row_position = scattering->nb_rows - 1;
  osl_int_t temp;
  osl_int_init_set_si(scattering->precision, &temp, grid_size);
  for (unsigned long i = 0; i < num_dimensions; ++i) {
    unsigned long initial_iterator_pos = 2 + 2*num_dimensions + i*2 ;
    unsigned long initial_scattering_pos = 2 + i*2;
    osl_int_increment(scattering->precision,
        &scattering->m[row_position][initial_iterator_pos],
        scattering->m[row_position][initial_iterator_pos]);
    osl_int_sub(scattering->precision,
        &scattering->m[row_position][initial_scattering_pos],
        scattering->m[row_position][initial_scattering_pos],
        temp);
  }
  unsigned long row_pos = (num_dimensions % 2) == 1 ?
    1 + 2*num_dimensions + 3*(num_dimensions-1)/2 :
    num_dimensions;
  osl_int_increment(scattering->precision,
      &scattering->m[row_pos][scattering->nb_columns-1],
      scattering->m[row_pos][scattering->nb_columns-1]);

  osl_int_clear(scattering->precision, &temp);
}

void acr_openscop_set_tiled_to_do_avg(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    const unsigned long *monitor_dim,
    osl_scop_p scop) {
  osl_statement_p init, add, div;
  init = scop->statement;
  add = osl_statement_clone(init);
  div = osl_statement_clone(init);
  acr_openscop_scan_init(monitor, filter_function, grid_size,
      acr_monitor_function_avg,
      monitor_dim, init);
  acr_openscop_scan_avg_add(monitor, filter_function, grid_size,
      monitor_dim, add);
  acr_openscop_scan_avg_div(monitor, grid_size,
      monitor_dim, div);
  init->next = add;
  add->next = div;
}

void acr_openscop_get_identifiers_with_dependencies(
    const acr_option monitor,
    const osl_scop_p scop,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used,
    osl_strings_p *all_iterators,
    osl_statement_p *statement_containing_them) {

  osl_strings_p identifiers = acr_openscop_get_monitor_identifiers(monitor);
  const size_t num_id = osl_strings_size(identifiers);
  char *const*ids = identifiers->string;
  osl_statement_p current_statement = scop->statement;
  bool found_all = false;
  unsigned long current_statement_num = 0;
  osl_body_p body = NULL;
  size_t num_iterators = 0;
  while (!found_all && current_statement) {
    body = osl_generic_lookup(current_statement->extension, OSL_URI_BODY);
    num_iterators = osl_strings_size(body->iterators);
    for (size_t i = 0; num_iterators >= num_id && i < num_id; ++i) {
      const size_t position = osl_strings_find(body->iterators, ids[i]);
      if (position >= num_iterators)
        break;
      if (i == num_id-1) {
        found_all = true;
      }
    }
    if (!found_all) {
      current_statement = current_statement->next;
      ++current_statement_num;
    }
  }
  if (!found_all) {
    *all_iterators = NULL;
    *statement_containing_them = NULL;
    *bound_used = NULL;
    return;
  }
  *bound_used = dims->statements_bounds[current_statement_num];
  *statement_containing_them = current_statement;
  bool *dims_left = malloc(num_iterators*sizeof(*dims_left));
  for (size_t i = 0; i < num_iterators; ++i) {
    dims_left[i] = false;
  }
  for (size_t i = 0; i < num_id; ++i) {
    const size_t position = osl_strings_find(body->iterators, ids[i]);
    dims_left[position] = true;
    for (unsigned long j = 0; j < position; ++j) {
      if(acr_osl_dim_has_constraints_with_dim(j, position,
            dims->statements_bounds[current_statement_num])) {
        dims_left[j] = true;
      }
    }
  }
  *all_iterators = osl_strings_malloc();
  for (size_t i = 0; i < num_iterators; ++i) {
    if (dims_left[i]) {
      osl_strings_add(*all_iterators, body->iterators->string[i]);
    }
  }
  free(dims_left);
  osl_strings_free(identifiers);
}

osl_relation_p acr_openscop_reduce_domain_to_id(
    const osl_relation_p initial_domain,
    const osl_strings_p initial_iterators,
    const osl_strings_p remaining_iterators) {
  osl_relation_p new_domain = osl_relation_clone(initial_domain);
  size_t num_initial_it = osl_strings_size(initial_iterators);
  size_t num_remaining_it = osl_strings_size(remaining_iterators);
  unsigned long deleted = 0;
  for (size_t i = 0; i < num_initial_it; ++i) {
    if (osl_strings_find(remaining_iterators, initial_iterators->string[i]) >=
        num_remaining_it) {
      unsigned long column_num = 1+i-deleted;
      for (int i = 0; i < new_domain->nb_rows; ++i) {
        if (!osl_int_zero(new_domain->precision, new_domain->m[i][column_num])){
          osl_relation_remove_row(new_domain, i);
          i--;
        }
      }
      osl_relation_remove_column(new_domain, column_num);
      new_domain->nb_output_dims -= 1;
      deleted += 1;
    }
  }
  return new_domain;
}

void acr_openscop_add_related_dims_in_scattering(
    osl_scop_p scop,
    dimensions_upper_lower_bounds *bounds) {
  osl_statement_p current_statement = scop->statement;
  bool *already_added = malloc(bounds->num_dimensions * sizeof(*already_added));
  while (current_statement) {
    osl_relation_p scattering = current_statement->scattering;

    for (unsigned long i = 0; i < bounds->num_dimensions; ++i) {
      already_added[i] = false;
      if (bounds->dimensions_type[i] == acr_dimension_type_bound_to_monitor) {
        already_added[i] = true;
        if (acr_osl_dim_has_constraints_with_previous_dims(i, bounds)) {
          bool remaining = i > 0;
          for (unsigned long j = i-1; remaining; --j) {
            if (!already_added[j] &&
                bounds->has_constraint_with_previous_dim[i][j]) {
              unsigned long where_to_add_in_dim = scattering->nb_columns - 1 -
                scattering->nb_parameters - scattering->nb_input_dims;
              unsigned long where_to_add_out_dim = 2;
              for (unsigned long k = 0; k <j; ++k) {
                if (already_added[k] || bounds->dimensions_type[k] ==
                    acr_dimension_type_bound_to_monitor) {
                  where_to_add_in_dim += 1;
                  where_to_add_out_dim += 2;
                }
              }
              osl_relation_insert_blank_column(scattering,
                  where_to_add_out_dim);
              osl_relation_insert_blank_column(scattering,
                  where_to_add_out_dim);
              where_to_add_in_dim += 2;
              osl_relation_insert_blank_column(scattering,
                  where_to_add_in_dim);
              osl_relation_insert_blank_row(scattering, -1);
              osl_int_increment(scattering->precision,
                  &scattering->m[scattering->nb_rows-1][where_to_add_in_dim],
                  scattering->m[scattering->nb_rows-1][where_to_add_in_dim]);
              where_to_add_in_dim += 1;
              osl_int_decrement(scattering->precision,
                  &scattering->m[scattering->nb_rows-1][where_to_add_out_dim],
                  scattering->m[scattering->nb_rows-1][where_to_add_out_dim]);
              osl_relation_insert_blank_row(scattering, -1);
              where_to_add_out_dim += 1;
              osl_int_decrement(scattering->precision,
                  &scattering->m[scattering->nb_rows-1][where_to_add_out_dim],
                  scattering->m[scattering->nb_rows-1][where_to_add_out_dim]);
              scattering->nb_input_dims += 1;
              scattering->nb_output_dims += 2;
              already_added[j] = true;
            }

            if (j == 0)
              remaining = false;
          }
        }
      }
    }
    current_statement = current_statement->next;
  }
  free(already_added);
}

void monitor_openscop_generate_monitor_dim(
    const dimensions_upper_lower_bounds *dims,
    unsigned long num_dims,
    unsigned long **monitor_dim) {
  bool *used_to_compute =
    malloc(dims->num_dimensions * sizeof(*used_to_compute));
  for (unsigned long i = 0; i < dims->num_dimensions; ++i) {
    used_to_compute[i] = false;
    if (dims->dimensions_type[i] == acr_dimension_type_bound_to_monitor) {
      used_to_compute[i] = true;
      for (unsigned long j = 0; j < i; ++j) {
        if (!used_to_compute[j] &&
            dims->has_constraint_with_previous_dim[i][j]) {
          used_to_compute[j] = true;
        }
      }
    }
  }
  unsigned long *mondim = malloc(num_dims * sizeof(*mondim));
  for (unsigned long i = 0; i < num_dims; ++i) {
    mondim[i] = 0;
    unsigned long current_dim = 0;
    for (unsigned long j = 0; j < dims->num_dimensions; ++j) {
      if (used_to_compute[j]) {
        mondim[i] += 1;
        if (dims->dimensions_type[j] == acr_dimension_type_bound_to_monitor) {
          if (current_dim == i) {
            break;
          } else {
            current_dim += 1;
          }
        }
      }
    }
  }
  free(used_to_compute);
  *monitor_dim = mondim;
}

osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const osl_scop_p scop,
    unsigned long grid_size,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used) {

  osl_strings_p all_identifiers;
  osl_statement_p statement;
  acr_openscop_get_identifiers_with_dependencies(
      monitor,
      scop,
      dims,
      bound_used,
      &all_identifiers,
      &statement);

  osl_body_p body = osl_generic_lookup(statement->extension, OSL_URI_BODY);
  osl_relation_p new_domain =
    acr_openscop_reduce_domain_to_id(
        statement->domain,
        body->iterators,
        all_identifiers);

  acr_array_declaration *decl = acr_monitor_get_array_declaration(monitor);
  unsigned long num_tiling_dim = acr_array_decl_get_num_dimensions(decl);

  osl_relation_p scattering =
    acr_openscop_new_scattering_relation(
        num_tiling_dim, new_domain->nb_parameters);
  acr_osl_apply_tiling(num_tiling_dim, scattering, grid_size);

  osl_statement_p new_statement = osl_statement_malloc();
  new_statement->domain = new_domain;
  new_statement->scattering = scattering;
  new_statement->next = NULL;

  osl_scop_p new_scop = osl_scop_malloc();
  new_scop->version = scop->version;
  new_scop->language = acr_strdup(scop->language);
  new_scop->registry = osl_interface_get_default_registry();
  new_scop->context = osl_relation_clone(scop->context);
  new_scop->parameters = osl_generic_clone(scop->parameters);
  new_scop->statement = new_statement;

  unsigned long *monitor_dim;
    monitor_openscop_generate_monitor_dim(*bound_used, num_tiling_dim,
        &monitor_dim);

  const char* filter = acr_monitor_get_filter_name(monitor);
  switch (acr_monitor_get_function(monitor)) {
    case acr_monitor_function_min:
      acr_openscop_set_tiled_to_do_min_max(
          monitor, filter, grid_size, false, monitor_dim, new_scop);
      break;
    case acr_monitor_function_max:
      acr_openscop_set_tiled_to_do_min_max(
          monitor, filter, grid_size, true, monitor_dim, new_scop);
      break;
    case acr_monitor_function_avg:
      acr_openscop_set_tiled_to_do_avg(
          monitor, filter, grid_size, monitor_dim, new_scop);
      break;
    case acr_monitor_function_unknown:
      break;
  }
  free(monitor_dim);

  acr_openscop_add_related_dims_in_scattering(new_scop, *bound_used);


  osl_strings_p scattering_names = osl_strings_generate("c",
      new_scop->statement->scattering->nb_output_dims);
  osl_scatnames_p scatnames = osl_scatnames_malloc();
  scatnames->names = scattering_names;
  new_scop->extension = osl_generic_shell(scatnames, osl_scatnames_interface());

  osl_strings_free(all_identifiers);

  return new_scop;
}

struct acr_get_min_max {
  dimensions_upper_lower_bounds *bounds;
  unsigned long current_dimension;
};

static isl_stat _acr_get_min_max_in_constraint(isl_constraint *co, void* user) {
  struct acr_get_min_max *wrapper = (struct acr_get_min_max*) user;
  dimensions_upper_lower_bounds *bounds = wrapper->bounds;
  unsigned long dimension = wrapper->current_dimension;
  if (isl_constraint_involves_dims(co, isl_dim_set, 0, 1)) {
    isl_bool lower_bound = isl_constraint_is_lower_bound(co, isl_dim_set, 0);
    for (unsigned long i = 0; i < bounds->num_parameters; ++i) {
      isl_val *dim_val =
        isl_constraint_get_coefficient_val(co, isl_dim_param, i);
      if (lower_bound == isl_bool_true) {
        if (isl_val_is_zero(dim_val) == isl_bool_false) {
          bounds->lower_bound[dimension][i] = true;
#ifdef ACR_DEBUG
          fprintf(stderr, "Dimension %lu depend on parameter %lu\n",
              dimension, i);
#endif
        }
      } else {
        if (lower_bound == isl_bool_false) {
          if (isl_val_is_zero(dim_val) == isl_bool_false) {
            bounds->upper_bound[dimension][i] = true;
#ifdef ACR_DEBUG
            fprintf(stderr, "Dimension %lu depend on parameter %lu\n",
                dimension, i);
#endif
          }
        } else {
          isl_val_free(dim_val);
          isl_constraint_free(co);
          return isl_stat_error;
        }
      }
      isl_val_free(dim_val);
    }
  }

  isl_constraint_free(co);
  return isl_stat_ok;
}

static isl_stat _acr_get_min_max_in_bset(isl_basic_set *bs, void* user) {
  isl_stat stat =
    isl_basic_set_foreach_constraint(bs, _acr_get_min_max_in_constraint, user);
  isl_basic_set_free(bs);
  return stat;
}
static isl_stat _acr_get_dimensions_dep_constraint(
    isl_constraint *co, void* user) {
  dimensions_upper_lower_bounds *bounds = (dimensions_upper_lower_bounds*) user;
  for (unsigned long dim = 1; dim < bounds->num_dimensions; ++dim) {
    for (unsigned long previous_dims = 0; previous_dims < dim; ++previous_dims){
      if (isl_constraint_involves_dims(co, isl_dim_set, dim, 1) &&
          isl_constraint_involves_dims(co, isl_dim_set, previous_dims, 1)) {
        bounds->has_constraint_with_previous_dim[dim][previous_dims] = true;
        if (previous_dims != 0) {
          for (unsigned long i = 0; i < previous_dims-1; ++i) {
            bounds->has_constraint_with_previous_dim[dim][i] =
              bounds->has_constraint_with_previous_dim[dim][i] ||
              bounds->has_constraint_with_previous_dim[previous_dims][i];
          }
        }
#ifdef ACR_DEBUG
        fprintf(stderr, "Dimension %lu has constraint with dim %lu\n",
            dim, previous_dims);
#endif
      }
    }
  }
  isl_constraint_free(co);
  return isl_stat_ok;
}

static isl_stat _acr_get_dimensions_dep_set(isl_basic_set *bs, void* user) {
  isl_stat stat =
    isl_basic_set_foreach_constraint(bs,
        _acr_get_dimensions_dep_constraint, user);
  isl_basic_set_free(bs);
  return stat;
}

dimensions_upper_lower_bounds* acr_osl_get_min_max_bound_statement(
    const osl_statement_p statement) {
  char* str;
  str = osl_relation_spprint_polylib(statement->domain, NULL);
  isl_ctx *ctx = isl_ctx_alloc();
  isl_set *domain = isl_set_read_from_str(ctx, str);
  free(str);
  dimensions_upper_lower_bounds *bounds = malloc(sizeof(*bounds));
  bounds->num_dimensions = isl_set_n_dim(domain);
  bounds->num_parameters = isl_set_n_param(domain);
  bounds->upper_bound =
    malloc(bounds->num_dimensions * sizeof(*bounds->upper_bound));
  bounds->lower_bound =
    malloc(bounds->num_dimensions * sizeof(*bounds->lower_bound));
  bounds->dimensions_type =
    malloc(bounds->num_dimensions * sizeof(*bounds->dimensions_type));
  bounds->has_constraint_with_previous_dim =
    malloc(bounds->num_dimensions *
        sizeof(*bounds->has_constraint_with_previous_dim));
  for (unsigned long i = 0; i < bounds->num_dimensions; ++i) {
    bounds->lower_bound[i] =
      malloc((bounds->num_parameters) * sizeof(**bounds->lower_bound));
    bounds->upper_bound[i] =
      malloc((bounds->num_parameters) * sizeof(**bounds->upper_bound));
    for (unsigned long j = 0 ; j <bounds->num_parameters; ++j) {
      bounds->lower_bound[i][j] = false;
      bounds->upper_bound[i][j] = false;
    }
    if (i == 0) {
      bounds->has_constraint_with_previous_dim[i] = NULL;
    } else {
      bounds->has_constraint_with_previous_dim[i] =
        malloc(i * sizeof(**bounds->has_constraint_with_previous_dim));
      for (unsigned long j = 0 ; j < i; ++j) {
        bounds->has_constraint_with_previous_dim[i][j] = false;
      }
    }
  }
  bounds->bound_lexmin = isl_set_copy(domain);
  bounds->bound_lexmin = isl_set_lexmin(bounds->bound_lexmin);
  bounds->bound_lexmax = isl_set_copy(domain);
  bounds->bound_lexmax = isl_set_lexmax(bounds->bound_lexmax);

  for (unsigned long dim = 0; dim < bounds->num_dimensions; ++dim) {
    isl_set *project_domain = isl_set_copy(domain);
    if (dim+1 < bounds->num_dimensions)
      project_domain = isl_set_project_out(project_domain, isl_dim_set,
          dim+1, bounds->num_dimensions - dim - 1);
    if (dim != 0)
      project_domain = isl_set_project_out(project_domain, isl_dim_set,
          0ul, dim);
    struct acr_get_min_max wrapper =
        { .current_dimension = dim, .bounds = bounds};
    isl_set_foreach_basic_set(project_domain,
        _acr_get_min_max_in_bset, (void*) &wrapper);
    isl_set_free(project_domain);
  }
  isl_set_foreach_basic_set(domain,
      _acr_get_dimensions_dep_set, (void*) bounds);
  isl_set_free(domain);
  return bounds;
}

dimensions_upper_lower_bounds_all_statements* acr_osl_get_upper_lower_bound_all(
        const osl_statement_p statement_list) {
  dimensions_upper_lower_bounds_all_statements *bounds_all =
    malloc(sizeof(*bounds_all));
  osl_statement_p statement_iterator = statement_list;
  bounds_all->num_statements = 0;
  while (statement_iterator) {
    bounds_all->num_statements += 1;
    statement_iterator = statement_iterator->next;
  }
  bounds_all->statements_bounds =
    malloc(bounds_all->num_statements * sizeof(*bounds_all->statements_bounds));
  statement_iterator = statement_list;
  unsigned long statement_num = 0ul;
  while (statement_iterator) {
    bounds_all->statements_bounds[statement_num] =
      acr_osl_get_min_max_bound_statement(statement_iterator);
    statement_iterator = statement_iterator->next;
    statement_num += 1;
  }
  return bounds_all;
}

void acr_osl_free_dimension_upper_lower_bounds(
    dimensions_upper_lower_bounds *bounds) {
  for (unsigned long i = 0; i < bounds->num_dimensions; ++i) {
    free(bounds->lower_bound[i]);
    free(bounds->upper_bound[i]);
    free(bounds->has_constraint_with_previous_dim[i]);
  }
  free(bounds->dimensions_type);
  free(bounds->upper_bound);
  free(bounds->lower_bound);
  free(bounds->has_constraint_with_previous_dim);
  isl_ctx *ctx = isl_set_get_ctx(bounds->bound_lexmin);
  isl_set_free(bounds->bound_lexmax);
  isl_set_free(bounds->bound_lexmin);
  isl_ctx_free(ctx);
  free(bounds);
}

void acr_osl_free_dimension_upper_lower_bounds_all(
    dimensions_upper_lower_bounds_all_statements *bounds_all) {
  for (unsigned long i = 0; i < bounds_all->num_statements; ++i) {
    acr_osl_free_dimension_upper_lower_bounds(bounds_all->statements_bounds[i]);
  }
  free(bounds_all->statements_bounds);
  free(bounds_all);
}

bool acr_osl_dim_has_constraints_with_dim(
    unsigned long dim1,
    unsigned long dim2,
    const dimensions_upper_lower_bounds *bounds) {
  if (dim1 > dim2) {
    unsigned long tempdim = dim1;
    dim1 = dim2;
    dim2 = tempdim;
  }
  if (dim2 >= bounds->num_dimensions) {
    return false;
  }
  if (dim1 == dim2)
    return true;
  else
    return bounds->has_constraint_with_previous_dim[dim2][dim1];
}

bool acr_osl_dim_has_constraints_with_previous_dims(
    unsigned long dim1,
    const dimensions_upper_lower_bounds *bounds) {
  if (dim1 >= bounds->num_dimensions)
    return false;
  bool has_anything_to_do_with_other = false;
  for (unsigned long i = 0; !has_anything_to_do_with_other && i < dim1; ++i) {
    has_anything_to_do_with_other =
      bounds->has_constraint_with_previous_dim[dim1][i];
  }
  return has_anything_to_do_with_other;
}

static bool _acr_osl_test_and_set_dims_type(
    const osl_statement_p statement,
    const osl_strings_p monitor_parameters_name,
    const osl_strings_p alternative_parameters,
    const osl_strings_p context_parameters,
    dimensions_upper_lower_bounds *bounds) {

  unsigned long num_monitor_dim = osl_strings_size(monitor_parameters_name);
  unsigned long num_alt_param = osl_strings_size(alternative_parameters);
  osl_body_p body = osl_generic_lookup(statement->extension, OSL_URI_BODY);
  char **iterators = body->iterators->string;
  unsigned long num_iterators = osl_strings_size(body->iterators);

  char **alt_params = alternative_parameters->string;
  for (unsigned long i = 0; i < num_iterators; ++i) {
    if (osl_strings_find(monitor_parameters_name,iterators[i])<num_monitor_dim){
      if (i > 0) {
        for (unsigned long j = 0; j < i ; ++j) {
          if (bounds->has_constraint_with_previous_dim[i][j] &&
              bounds->dimensions_type[j] == acr_dimension_type_bound_to_alternative) {
            fprintf(stderr,
                "[ACR] error: Dimension described by iterator %s depends on"
                " dimension represented by iterator %s.\n"
                "             This dimension is bounded by an alternative"
                " parameter and can not be used to represent the monitoring domain.\n"
                , iterators[i], iterators[j]);
            return false;
          }
        }
      }
      for (unsigned long j = 0; j < num_alt_param; ++j) {
        unsigned long param_pos =
          osl_strings_find(context_parameters, alt_params[j]);
        if (bounds->upper_bound[i][param_pos] == true ||
            bounds->lower_bound[i][param_pos] == true) {
          fprintf(stderr,
              "[ACR] error: Dimension described by iterator %s depends on"
              " parameter %s.\n  "
              "           The same parameter is used in alternative parameter\n"
              "             construct and can not also be used to represent the"
              " monitoring domain.\n"
              , iterators[i], alt_params[j]);
          return false;
        }
      }
      bounds->dimensions_type[i] = acr_dimension_type_bound_to_monitor;
    } else { // not monitoring
      bool has_alt_param = false;
      for (unsigned long j = 0; !has_alt_param && j < num_alt_param; ++j) {
        unsigned long param_pos =
          osl_strings_find(context_parameters, alt_params[j]);
        if (bounds->upper_bound[i][param_pos] == true ||
            bounds->lower_bound[i][param_pos] == true) {
          bounds->dimensions_type[i] = acr_dimension_type_bound_to_alternative;
          has_alt_param = true;
        }
      }
      if (!has_alt_param) {
        if (i > 0) {
          bool has_constraint = false;
          for (unsigned long j = 0; !has_constraint && j < i ; ++j) {
            if (bounds->has_constraint_with_previous_dim[i][j]) {
              switch (bounds->dimensions_type[j]) {
                case acr_dimension_type_bound_to_alternative:
                  bounds->dimensions_type[i] = bounds->dimensions_type[j];
                  has_constraint = true;
                  break;
                case acr_dimension_type_bound_to_monitor:
                case acr_dimension_type_free_dim:
                  break;
              }
            }
          }
          if (!has_constraint) {
            bounds->dimensions_type[i] = acr_dimension_type_free_dim;
          }
        } else {
          bounds->dimensions_type[i] = acr_dimension_type_free_dim;
        }
      }
    }
  }
  return true;
}

bool acr_osl_find_and_verify_free_dims_position(
    const acr_compute_node node,
    const osl_scop_p scop,
    dimensions_upper_lower_bounds_all_statements *bounds_all) {
  osl_strings_p pragma_alternative_parameters = osl_strings_malloc();
  size_t string_size = 0;
  osl_strings_p pragma_monitor_iterators = NULL;
  acr_option_list opt_list = acr_compute_node_get_option_list(node);
  unsigned long list_size = acr_compute_node_get_option_list_size(node);
  for (unsigned long i = 0; i < list_size; ++i) {
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
    } else {
      if (acr_option_get_type(current_option) == acr_type_monitor) {
        pragma_monitor_iterators =
          acr_openscop_get_monitor_identifiers(current_option);
        if (acr_osl_check_if_identifiers_are_not_parameters(
              scop, pragma_monitor_iterators)) {
          fprintf(stdout,
              "[ACR] error: parameters used in array declaration:\n");
          pprint_acr_option(stderr, current_option, 0);
          osl_strings_free(pragma_monitor_iterators);
          osl_strings_free(pragma_alternative_parameters);
          return false;
        }
      }
    }
  }
  osl_statement_p current_statement = scop->statement;
  bool valid = true;
  osl_strings_p context_parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);
  for (unsigned long i = 0; valid && i < bounds_all->num_statements; ++i) {
    valid = _acr_osl_test_and_set_dims_type(
        current_statement,
        pragma_monitor_iterators,
        pragma_alternative_parameters,
        context_parameters,
        bounds_all->statements_bounds[i]);
    current_statement = current_statement->next;
  }

  osl_strings_free(pragma_monitor_iterators);
  osl_strings_free(pragma_alternative_parameters);
  return valid;
}
