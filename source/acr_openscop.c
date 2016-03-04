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

static void _acr_openscop_add_missing_parameters(unsigned long* size_string,
    osl_strings_p parameters,
    const acr_array_dimension dimension) {
  if (dimension->type != acr_array_dim_leaf) {
    _acr_openscop_add_missing_parameters(size_string, parameters,
        dimension->val.node.left);
    _acr_openscop_add_missing_parameters(size_string, parameters,
        dimension->val.node.right);
  } else {
    switch (dimension->val.leaf.type) {
      case acr_expr_leaf_int:
        break;
      case acr_expr_leaf_param:
        if (osl_strings_find(parameters, dimension->val.leaf.value.parameter)
            == *size_string) {
          osl_strings_add(parameters, dimension->val.leaf.value.parameter);
          *size_string += 1;
        }
        break;
    }
  }
}

osl_strings_p acr_openscop_get_monitor_parameters(const acr_option monitor) {
  size_t string_size = 0;
  osl_strings_p parameters = osl_strings_malloc();
  acr_array_declaration* array_decl =
    acr_monitor_get_array_declaration(monitor);
  acr_array_dimensions_list dim_list = array_decl->array_dimensions_list;
  for (unsigned long i = 0; i < array_decl->num_dimensions; ++i) {
    _acr_openscop_add_missing_parameters(&string_size, parameters, dim_list[i]);
  }
  return parameters;
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

osl_relation_p acr_openscop_domain_from_monitor(
    const acr_option monitor,
    size_t num_dimensions,
    size_t num_parameters,
    const osl_strings_p parameters) {
  osl_relation_p new_relation = osl_relation_malloc(num_dimensions * 2,
      num_dimensions + num_parameters + 2);
  osl_relation_set_type(new_relation, OSL_TYPE_DOMAIN);
  osl_relation_set_attributes(new_relation, num_dimensions, 0, 0, num_parameters);
  acr_array_declaration* decl = acr_monitor_get_array_declaration(monitor);
  acr_array_dimensions_list dim_list = acr_array_decl_get_dimensions_list(decl);
  for (size_t i = 0; i < num_dimensions; ++i) {
    acr_osl_update_lower_bound_from_dimension(
        new_relation, i);
    bool updated = acr_osl_update_upper_bound_from_dimension(
        new_relation, i, dim_list[i], parameters);
    if (!updated) {
      fprintf(stderr, "[ACR] error: Malformed monitor:\n");
      pprint_acr_option(stderr, monitor, 0);
      return NULL;
    }
  }
  return new_relation;
}

bool acr_osl_check_if_parameters_are_also_iterator(const osl_scop_p scop,
    osl_strings_p parameters) {
  size_t num_parameters = osl_strings_size(parameters);
  osl_statement_p statements = scop->statement;
  while (statements) {
    osl_body_p body = osl_generic_lookup(statements->extension, OSL_URI_BODY);
    size_t num_iterators = osl_strings_size(body->iterators);
    for (size_t i = 0; i < num_parameters; ++i) {
      if (osl_strings_find(body->iterators, parameters->string[i]) < num_iterators) {
        return true;
      }
    }
    statements = statements->next;
  }
  return false;
}

static void acr_openscop_scan_min_max_init(
    const acr_option monitor,
    const char* filter_function,
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
  fprintf(tempbuffer, "tempval = ");
  if (filter_function) {
    fprintf(tempbuffer, "%s(", filter_function);
  }
  fprintf(tempbuffer, "%s", acr_array_decl_get_array_name(decl));
  if (filter_function) {
    fprintf(tempbuffer, ")");
  }
  char **current_string = iterators->string;
  while (*current_string) {
    fprintf(tempbuffer, "[%s]", *current_string);
    current_string++;
  }
  fprintf(tempbuffer, ";");
  fclose(tempbuffer);
  body->expression = osl_strings_encapsulate(statement_string);
  osl_generic_add(&statement->extension,
      osl_generic_shell(body, osl_body_interface()));
}

void acr_openscop_set_tiled_to_do_min_max(
    const acr_option monitor,
    const char* filter_function,
    unsigned long grid_size,
    bool max,
    osl_scop_p scop) {
  osl_statement_p init, inf_or_sup, end;
  init = scop->statement;
  /*inf_or_sup = osl_statement_clone(init);*/
  /*end = osl_statement_clone(init);*/
  acr_openscop_scan_min_max_init(monitor, filter_function, max, init);
}

osl_scop_p acr_openscop_gen_monitor_loop(const acr_option monitor,
    const osl_scop_p scop,
    unsigned long grid_size) {

  osl_strings_p parameters = acr_openscop_get_monitor_parameters(monitor);
  if (acr_osl_check_if_parameters_are_also_iterator(scop, parameters)) {
    fprintf(stdout, "[ACR] error: iterators used in array declaration:\n");
    pprint_acr_option(stderr, monitor, 0);
    osl_strings_free(parameters);
    return NULL;
  }

  size_t num_param = osl_strings_size(parameters);
  size_t num_dimensions = acr_monitor_num_dimensions(monitor);

  osl_relation_p domain =
    acr_openscop_domain_from_monitor(monitor, num_dimensions, num_param,
        parameters);
  if (!domain) {
    osl_strings_free(parameters);
    return NULL;
  }
  osl_relation_p scattering =
    acr_openscop_new_scattering_relation(num_dimensions, num_param);
  acr_osl_apply_tiling(num_dimensions, scattering, grid_size);

  osl_statement_p new_statement = osl_statement_malloc();
  new_statement->domain = domain;
  new_statement->scattering = scattering;

  osl_scop_p new_scop = osl_scop_malloc();
  new_scop->version = scop->version;
  new_scop->language = acr_strdup(scop->language);
  osl_generic_p generic_string = osl_generic_shell(parameters, osl_strings_interface());
  osl_generic_add(&new_scop->parameters, generic_string);
  new_scop->statement = new_statement;
  new_scop->context = osl_relation_malloc(0,num_param+2);
  osl_relation_set_type(new_scop->context, OSL_TYPE_CONTEXT);
  osl_relation_set_attributes(new_scop->context, 0, 0, 0, num_param);
  return new_scop;
}
