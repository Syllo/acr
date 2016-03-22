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

osl_scop_p acr_read_scop_from_buffer(char* buffer, size_t size_buffer) {
  FILE* fakefile = fmemopen(buffer, size_buffer, "r");
  osl_scop_p scop = osl_scop_read(fakefile);
  fclose(fakefile);
  return scop;
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

static void _acr_openscop_add_missing_identifiers(unsigned long* size_string,
    osl_strings_p identifiers,
    const acr_array_dimension dimension) {
  if (dimension->type != acr_array_dim_leaf) {
    _acr_openscop_add_missing_identifiers(size_string, identifiers,
        dimension->val.node.left);
    _acr_openscop_add_missing_identifiers(size_string, identifiers,
        dimension->val.node.right);
  } else {
    switch (dimension->val.leaf.type) {
      case acr_expr_leaf_int:
        break;
      case acr_expr_leaf_param:
        if (osl_strings_find(identifiers, dimension->val.leaf.value.parameter)
            == *size_string) {
          osl_strings_add(identifiers, dimension->val.leaf.value.parameter);
          *size_string += 1;
        }
        break;
    }
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

static void acr_osl_update_lower_bound_from_dimension(
    osl_relation_p relation,
    unsigned long which_dim) {
  osl_int_increment(relation->precision,
      &relation->m[which_dim*2][0],
      relation->m[which_dim*2][0]);
  osl_int_increment(relation->precision,
      &relation->m[which_dim*2][which_dim + 1],
      relation->m[which_dim*2][which_dim + 1]);
}

static bool acr_array_dim_get_identifier(const acr_array_dimension dim,
    char** identifier) {
  char* left_id, *right_id;
  bool left_retval, right_retval;
  switch (dim->type) {
    case acr_array_dim_leaf:
      switch (dim->val.leaf.type) {
        case acr_expr_leaf_int:
          *identifier = NULL;
          return false;
          break;
        case acr_expr_leaf_param:
          *identifier = dim->val.leaf.value.parameter;
          return true;
          break;
      }
      break;
    case acr_array_dim_mul:
    case acr_array_dim_div:
    case acr_array_dim_plus:
    case acr_array_dim_minus:
      left_retval =
        acr_array_dim_get_identifier(dim->val.node.left, &left_id);
      right_retval =
        acr_array_dim_get_identifier(dim->val.node.right, &right_id);
      if(right_retval && left_retval) {
        *identifier = NULL;
        return true;
      }
      if (left_retval && left_id == NULL) {
        *identifier = NULL;
        return true;
      } else {
        if (right_retval && right_id == NULL) {
          *identifier = NULL;
          return true;
        } else {
          if (right_retval) {
            *identifier = right_id;
            return true;
          } else {
            *identifier = left_id;
            return true;
          }
        }
      }
      *identifier = NULL;
      return false;
      break;
  }
  *identifier = NULL;
  return false;
}


static bool acr_osl_update_dim(
    osl_relation_p relation,
    unsigned long which_dim,
    const acr_array_dimension dim,
    const osl_strings_p parameters,
    bool plus);

static bool acr_osl_update_leaf(
    osl_relation_p relation,
    unsigned long which_dim,
    const acr_array_dimension dim,
    const osl_strings_p parameters,
    bool plus,
    const char* parameter) {

  size_t parameter_position;
  int column_position;
  int row_position = which_dim * 2 + 1;
  osl_int_p temp_val = osl_int_malloc(relation->precision);

  switch(dim->val.leaf.type) {
    case acr_expr_leaf_int:
      osl_int_set_si(relation->precision,
          temp_val,
          dim->val.leaf.value.integer);
      if (!plus) {
        osl_int_oppose(relation->precision,
            temp_val,
            *temp_val);
      }
      if (parameter) {
        parameter_position = osl_strings_find(parameters, parameter);
        column_position = 1 + relation->nb_output_dims + parameter_position;
        if (osl_int_zero(relation->precision,
              relation->m[row_position][column_position])) {
          osl_int_increment(relation->precision,
              &relation->m[row_position][column_position],
              relation->m[row_position][column_position]);
        }
        osl_int_mul(relation->precision,
            &relation->m[row_position][column_position],
            relation->m[row_position][column_position],
            *temp_val);
      } else {
        column_position = relation->nb_columns - 1;
        osl_int_add(relation->precision,
            &relation->m[row_position][column_position],
            relation->m[row_position][column_position],
            *temp_val);
      }
      break;
    case acr_expr_leaf_param:
      if (!parameter) {
        parameter_position =
          osl_strings_find(parameters, dim->val.leaf.value.parameter);
        column_position = 1 + relation->nb_output_dims + parameter_position;
        if (plus) {
          osl_int_increment(relation->precision,
              &relation->m[row_position][column_position],
              relation->m[row_position][column_position]);
        } else {
          osl_int_decrement(relation->precision,
              &relation->m[row_position][column_position],
              relation->m[row_position][column_position]);
        }
      }
      break;
  }
  osl_int_free(relation->precision, temp_val);
  return true;
}

static bool acr_osl_update_mul(
    osl_relation_p relation,
    unsigned long which_dim,
    const acr_array_dimension dim,
    const osl_strings_p parameters,
    bool plus,
    char* found_id) {
  char* param;
  if (found_id) {
    param = found_id;
  } else {
    bool found_param = acr_array_dim_get_identifier(dim, &param);
    if (found_param && param == NULL) {
      fprintf(stderr, "[ACR] error: Non affine expression: ");
      print_acr_array_dimensions(stderr, dim, false);
      fprintf(stderr, "\n");
      return false;
    }
  }

  bool left_up, right_up;
  acr_array_dimension left_hand_side, right_hand_side;
  left_hand_side = dim->val.node.left;
  right_hand_side = dim->val.node.right;

  switch (left_hand_side->type) {
    case acr_array_dim_plus:
    case acr_array_dim_minus:
    case acr_array_dim_div:
      return false;
      break;
    case acr_array_dim_mul:
      left_up = acr_osl_update_mul(relation,
            which_dim,
            left_hand_side,
            parameters,
            plus,
            param);
      break;
    case acr_array_dim_leaf:
      left_up = acr_osl_update_leaf(relation,
            which_dim,
            left_hand_side,
            parameters,
            plus,
            param);
      break;
    default:
      left_up = false;
      break;
  }

  switch (right_hand_side->type) {
    case acr_array_dim_plus:
    case acr_array_dim_minus:
    case acr_array_dim_div:
      return false;
      break;
    case acr_array_dim_mul:
      right_up = acr_osl_update_mul(relation,
            which_dim,
            right_hand_side,
            parameters,
            plus,
            param);
      break;
    case acr_array_dim_leaf:
      right_up = acr_osl_update_leaf(relation,
          which_dim,
          right_hand_side,
          parameters,
          plus,
          param);
      break;
    default:
      right_up = false;
      break;
  }
  return right_up && left_up;
}

static bool acr_osl_update_plus_minus(
    osl_relation_p relation,
    unsigned long which_dim,
    const acr_array_dimension dim,
    const osl_strings_p parameters,
    bool plus) {
  bool left_up, right_up;
  acr_array_dimension left_hand_side, right_hand_side;
  left_hand_side = dim->val.node.left;
  right_hand_side = dim->val.node.right;

  switch (left_hand_side->type) {
    case acr_array_dim_plus:
    case acr_array_dim_minus:
      left_up = acr_osl_update_plus_minus(relation,
          which_dim,
          left_hand_side,
          parameters,
          plus);
      break;
    case acr_array_dim_leaf:
      left_up = acr_osl_update_leaf(relation,
          which_dim,
          left_hand_side,
          parameters,
          plus,
          NULL);
      break;
    case acr_array_dim_mul:
      left_up = acr_osl_update_mul(relation,
          which_dim,
          left_hand_side,
          parameters,
          plus,
          NULL);
      break;
    case acr_array_dim_div:
      left_up = false;
      break;
    default:
      left_up = false;
      break;
  }

  switch (right_hand_side->type) {
    case acr_array_dim_plus:
    case acr_array_dim_minus:
      if (dim->type == acr_array_dim_minus) {
        right_up = acr_osl_update_plus_minus(relation,
            which_dim,
            right_hand_side,
            parameters,
            false);
      } else {
        right_up = acr_osl_update_plus_minus(relation,
            which_dim,
            right_hand_side,
            parameters,
            true);
      }
      break;
    case acr_array_dim_leaf:
      if (dim->type == acr_array_dim_minus) {
        right_up = acr_osl_update_leaf(relation,
            which_dim,
            right_hand_side,
            parameters,
            false,
            NULL);
      } else {
        right_up = acr_osl_update_leaf(relation,
            which_dim,
            right_hand_side,
            parameters,
            true,
            NULL);
      }
      break;
    case acr_array_dim_mul:
      if (dim->type == acr_array_dim_minus) {
        right_up = acr_osl_update_mul(relation,
            which_dim,
            right_hand_side,
            parameters,
            false,
            NULL);
      } else {
        right_up = acr_osl_update_mul(relation,
            which_dim,
            right_hand_side,
            parameters,
            true,
            NULL);
      }
      break;
    case acr_array_dim_div:
      right_up = false;
      break;
    default:
      right_up = false;
      break;
  }
  return left_up && right_up;
}

static bool acr_osl_update_dim(
    osl_relation_p relation,
    unsigned long which_dim,
    const acr_array_dimension dim,
    const osl_strings_p parameters,
    bool plus) {
  switch (dim->type) {
    case acr_array_dim_leaf:
      return acr_osl_update_leaf(relation,
          which_dim,
          dim,
          parameters,
          plus,
          NULL);
      break;
    case acr_array_dim_plus:
      return acr_osl_update_plus_minus(relation,
          which_dim,
          dim,
          parameters,
          plus);
      break;
    case acr_array_dim_minus:
      return acr_osl_update_plus_minus(relation,
          which_dim,
          dim,
          parameters,
          plus);
      break;
    case acr_array_dim_mul:
      return acr_osl_update_mul(relation,
          which_dim,
          dim,
          parameters,
          plus,
          NULL);
      break;
    case acr_array_dim_div:
      return false;
      break;
  }
  return false;
}


static bool acr_osl_update_upper_bound_from_dimension(
    osl_relation_p relation,
    unsigned long which_dim,
    const acr_array_dimension dim,
    const osl_strings_p parameters) {
  osl_int_increment(relation->precision,
      &relation->m[which_dim*2+1][0],
      relation->m[which_dim*2+1][0]);
  osl_int_decrement(relation->precision,
      &relation->m[which_dim*2+1][1+which_dim],
      relation->m[which_dim*2+1][1+which_dim]);
  osl_int_decrement(relation->precision,
      &relation->m[which_dim*2+1][relation->nb_columns - 1],
      relation->m[which_dim*2+1][relation->nb_columns - 1]);
  return acr_osl_update_dim(
      relation,
      which_dim,
      dim,
      parameters,
      true);
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

static void acr_openscop_scan_min_max_op(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    const char* data_location_prefix,
    bool max,
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
  fprintf(tempbuffer, "%s_monitor_result", data_location_prefix);
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[c%lu]", (i+1)*2);
  }
  fprintf(tempbuffer, " = ");
  fprintf(tempbuffer, "%s_monitor_result", data_location_prefix);
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[c%lu]", (i+1)*2);
  }
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
  char **current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, "[%s]", *current_string);
    current_string++;
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, " ? ");
  if (filter_function) {
    fprintf(tempbuffer, "%s(", filter_function);
  }
  fprintf(tempbuffer, "%s", acr_array_decl_get_array_name(decl));
  current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, "[%s]", *current_string);
    current_string++;
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, " : ");
  fprintf(tempbuffer, "%s_monitor_result", data_location_prefix);
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[c%lu]", (i+1)*2);
  }
  fprintf(tempbuffer, ";");
#else
  (void) filter_function;
  (void) data_location_prefix;
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
    fprintf(tempbuffer, ", c%lu", (i+1)*2);
  }
  char **current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, ", %s", *current_string);
    current_string++;
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
    const char* data_location_prefix,
    enum acr_monitor_processing_funtion process_fun,
    const osl_strings_p identifiers,
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
      fprintf(tempbuffer, "%s_monitor_result", data_location_prefix);
      for(unsigned long i = 0; i < num_dimensions; ++i) {
        fprintf(tempbuffer, "[c%lu]", (i+1)*2);
      }
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
  char **current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, "[%s]", *current_string);
    current_string++;
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, ";");
#else
  (void) filter_function;
  (void) data_location_prefix;
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
    fprintf(tempbuffer, ", c%lu", (i+1)*2);
  }
  char **current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, ", %s", *current_string);
    current_string++;
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
    const char* data_location_prefix,
    const osl_strings_p identifiers,
    osl_scop_p scop) {
  osl_statement_p init, inf_or_sup;
  init = scop->statement;
  inf_or_sup = osl_statement_clone(init);
  acr_openscop_scan_init(monitor, filter_function, grid_size,
      data_location_prefix, acr_monitor_function_max, identifiers, init);
  acr_openscop_scan_min_max_op(monitor, filter_function, grid_size,
      data_location_prefix, max, inf_or_sup);
  scop->statement->next = inf_or_sup;
}

static void acr_openscop_scan_avg_add(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    const osl_strings_p identifiers,
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
  char **current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, "[%s]", *current_string);
    current_string++;
  }
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  fprintf(tempbuffer, ";");
#else
  (void) filter_function;
  fprintf(tempbuffer, "fprintf(stderr, \"Avg add ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "= ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "\\n\"");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", c%lu", (i+1)*2);
  }
  char **current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, ", %s", *current_string);
    current_string++;
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
    const char* data_location_prefix,
    const osl_strings_p identifiers,
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
  fprintf(tempbuffer, "%s_monitor_result", data_location_prefix);
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "[c%lu]", (i+1)*2);
  }
  unsigned long num_elements_grid = grid_size;
  for(unsigned int i = 1; i < num_dimensions; ++i) {
    num_elements_grid *= grid_size;
  }
  fprintf(tempbuffer, " = temp_avg / num_value;");
#else
  (void) data_location_prefix;
  fprintf(tempbuffer, "fprintf(stderr, \"Avg div ");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, "%%d ");
  }
  fprintf(tempbuffer, "by %%zu\\n\"");
  for(unsigned long i = 0; i < num_dimensions; ++i) {
    fprintf(tempbuffer, ", c%lu", (i+1)*2);
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
    const char* data_location_prefix,
    const osl_strings_p identifiers,
    osl_scop_p scop) {
  osl_statement_p init, add, div;
  init = scop->statement;
  add = osl_statement_clone(init);
  div = osl_statement_clone(init);
  acr_openscop_scan_init(monitor, filter_function, grid_size,
      data_location_prefix, acr_monitor_function_avg, identifiers, init);
  acr_openscop_scan_avg_add(monitor, filter_function, grid_size,
      identifiers, add);
  acr_openscop_scan_avg_div(monitor, grid_size,
      data_location_prefix, identifiers, div);
  init->next = add;
  add->next = div;
}

void acr_openscop_get_identifiers_with_dependencies(
    const osl_strings_p id,
    const osl_scop_p scop,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used,
    osl_strings_p *all_iterators,
    osl_statement_p *statement_containing_them) {
  const size_t num_id = osl_strings_size(id);
  char *const*ids = id->string;
  osl_statement_p current_statement = scop->statement;
  bool found_all = false;
  unsigned long current_statement_num = 0;
  osl_body_p body;
  size_t num_iterators;
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
  *bound_used = dims->statements_bounds[current_statement_num];
  *all_iterators = osl_strings_malloc();
  for (size_t i = 0; i < num_iterators; ++i) {
    if (dims_left[i]) {
      osl_strings_add(*all_iterators, body->iterators->string[i]);
    }
  }
  free(dims_left);
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

osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const char *prefix,
    const osl_scop_p scop,
    unsigned long grid_size,
    const dimensions_upper_lower_bounds_all_statements *dims,
    dimensions_upper_lower_bounds **bound_used) {

  osl_strings_p identifiers = acr_openscop_get_monitor_identifiers(monitor);

  osl_strings_p all_identifiers;
  osl_statement_p statement;
  acr_openscop_get_identifiers_with_dependencies(
      identifiers,
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

  size_t num_tiling_dim = osl_strings_size(identifiers);
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

  const char* filter = acr_monitor_get_filter_name(monitor);
  switch (acr_monitor_get_function(monitor)) {
    case acr_monitor_function_min:
      acr_openscop_set_tiled_to_do_min_max(
          monitor, filter, grid_size, false, prefix, identifiers, new_scop);
      break;
    case acr_monitor_function_max:
      acr_openscop_set_tiled_to_do_min_max(
          monitor, filter, grid_size, true, prefix, identifiers, new_scop);
      break;
    case acr_monitor_function_avg:
      acr_openscop_set_tiled_to_do_avg(
          monitor, filter, grid_size, prefix, identifiers, new_scop);
      break;
    case acr_monitor_function_unknown:
      break;
  }

  osl_scop_print(stderr, new_scop);

  osl_strings_free(identifiers);
  osl_strings_free(all_identifiers);
  // PLACEHOLDER
  return new_scop;

  osl_strings_p scattering_names = osl_strings_generate("c",
      new_scop->statement->scattering->nb_output_dims);
  osl_scatnames_p scatnames = osl_scatnames_malloc();
  scatnames->names = scattering_names;
  new_scop->extension = osl_generic_shell(scatnames, osl_scatnames_interface());

  return new_scop;
}

struct acr_get_min_max {
  dimensions_upper_lower_bounds *bounds;
  unsigned long current_dimension;
};

#ifdef ACR_DEBUG
static void pprint_isl_set(isl_set *set, const char* print) {
  fprintf(stderr, "PPRINT SET %s\n", print);
  isl_ctx *ctx = isl_set_get_ctx(set);
  isl_printer *splinter = isl_printer_to_file(ctx, stderr);
  isl_printer_print_set(splinter, set);
  isl_printer_flush(splinter);
  isl_printer_free(splinter);
  fprintf(stderr, "\nEND PPRINT SET %s\n", print);
}

/*static void pprint_isl_constraint(isl_constraint *co, const char* print) {*/
  /*fprintf(stderr, "PPRINT CONSTRAINT %s\n", print);*/
  /*isl_ctx *ctx = isl_constraint_get_ctx(co);*/
  /*isl_printer *splinter = isl_printer_to_file(ctx, stderr);*/
  /*isl_printer_print_constraint(splinter, co);*/
  /*isl_printer_flush(splinter);*/
  /*isl_printer_free(splinter);*/
  /*fprintf(stderr, "\nEND PPRINT CONSTRAINT %s\n", print);*/
/*}*/
#endif

static isl_stat _acr_get_min_max_in_constraint(isl_constraint *co, void* user) {
  struct acr_get_min_max *wrapper = (struct acr_get_min_max*) user;
  dimensions_upper_lower_bounds *bounds = wrapper->bounds;
  unsigned long dimension = wrapper->current_dimension;
  if (isl_constraint_involves_dims(co, isl_dim_set, 0, 1)) {
    isl_bool lower_bound = isl_constraint_is_lower_bound(co, isl_dim_set, 0);
#ifdef ACR_DEBUG
    /*if (lower_bound == isl_bool_true) {*/
      /*pprint_isl_constraint(co, "LOWER");*/
    /*} else {*/
        /*if (lower_bound == isl_bool_false) {*/
          /*pprint_isl_constraint(co, "UPPPER");*/
        /*} else {*/
          /*fprintf(stderr, "ERROR\n");*/
        /*}*/
    /*}*/
#endif
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
#ifdef ACR_DEBUG
    /*pprint_isl_set(project_domain, "BEFOR PROJECT");*/
#endif
    if (dim+1 < bounds->num_dimensions)
      project_domain = isl_set_project_out(project_domain, isl_dim_set,
          dim+1, bounds->num_dimensions - dim - 1);
    if (dim != 0)
      project_domain = isl_set_project_out(project_domain, isl_dim_set,
          0ul, dim);
#ifdef ACR_DEBUG
    /*pprint_isl_set(project_domain, "AFTER PROJECT");*/
    /*isl_set *project_domainlexmin = isl_set_copy(domain);*/
    /*project_domainlexmin = isl_set_lexmin(project_domainlexmin);*/
    /*pprint_isl_set(project_domainlexmin, "AFTER LEXMIN");*/
    /*isl_set *project_domainlexmax = isl_set_copy(domain);*/
    /*isl_set_print_internal(project_domainlexmin, stderr, 0);*/
    /*project_domainlexmax = isl_set_lexmax(project_domainlexmax);*/
    /*pprint_isl_set(project_domainlexmax, "AFTER LEXMAX");*/
    /*isl_set_print_internal(project_domainlexmax, stderr, 0);*/
    /*isl_set_free(project_domainlexmax);*/
    /*isl_set_free(project_domainlexmin);*/
#endif
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
  isl_set_free(bounds->bound_lexmax);
  isl_set_free(bounds->bound_lexmin);
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

  fprintf(stderr, "Real iterators\n");
  osl_strings_print(stderr, body->iterators);

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
#ifdef ACR_DEBUG
      fprintf(stderr, "Dim %s is bound to monitor\n", iterators[i]);
#endif
    } else { // not monitoring
      bool has_alt_param = false;
      for (unsigned long j = 0; !has_alt_param && j < num_alt_param; ++j) {
        unsigned long param_pos =
          osl_strings_find(context_parameters, alt_params[j]);
        if (bounds->upper_bound[i][param_pos] == true ||
            bounds->lower_bound[i][param_pos] == true) {
          bounds->dimensions_type[i] = acr_dimension_type_bound_to_alternative;
          has_alt_param = true;
#ifdef ACR_DEBUG
          fprintf(stderr, "Dim %s is bound to alternative\n", iterators[i]);
#endif
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
#ifdef ACR_DEBUG
                  fprintf(stderr, "Dim %s is bound to alternative\n",
                      iterators[i]);
#endif
                  break;
                case acr_dimension_type_bound_to_monitor:
                case acr_dimension_type_free_dim:
                  break;
              }
            }
          }
          if (!has_constraint) {
            bounds->dimensions_type[i] = acr_dimension_type_free_dim;
#ifdef ACR_DEBUG
            fprintf(stderr, "Dim %s is free\n", iterators[i]);
#endif
          }
        } else {
          bounds->dimensions_type[i] = acr_dimension_type_free_dim;
#ifdef ACR_DEBUG
          fprintf(stderr, "Dim %s is free\n", iterators[i]);
#endif
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
  fprintf(stderr, "Iterators\n");
  osl_strings_print(stderr, pragma_monitor_iterators);
  fprintf(stderr, "Alternative Parameters\n");
  osl_strings_print(stderr, pragma_alternative_parameters);

  osl_statement_p current_statement = scop->statement;
  bool valid = true;
  osl_strings_p context_parameters =
    osl_generic_lookup(scop->parameters, OSL_URI_STRINGS);
  fprintf(stderr, "Context Parameters\n");
  osl_strings_print(stderr, context_parameters);
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
