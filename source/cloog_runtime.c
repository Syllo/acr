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

#include <acr/cloog_runtime.h>

#include <string.h>

#include <cloog/domain.h>
#include <cloog/isl/domain.h>
#include <cloog/isl/backend.h>

#include <isl/map.h>

#include <osl/body.h>
#include <osl/strings.h>

static char* reprep(char* initial, const char* torep, const char* rep) {
  char* subpos;
  size_t sizerep = strlen(rep);
  size_t sizetorep = strlen(torep);
  do {
    subpos = strstr(initial, torep);
    if (subpos) {
      size_t size_initial = strlen(initial);
      size_t remaining_space = strlen(subpos) - sizetorep;
      if (sizerep > sizetorep) {
        initial = realloc(initial,
            (size_initial+1+sizerep-sizetorep) * sizeof(*initial));
        subpos = strstr(initial, torep);
      }
      memmove(subpos+sizerep, subpos+sizetorep, remaining_space+1);
      memcpy(subpos, rep, sizerep);
      if (sizerep < sizetorep) {
        initial = realloc(initial,
            (size_initial+1-(sizetorep-sizerep)) * sizeof(*initial));
      }
    }
  } while (subpos);
  return initial;
}

static osl_body_p _acr_body_copy_and_set_alternative_function(
    osl_body_p body,
    struct runtime_alternative *alternative) {
  osl_body_p new_body = osl_body_malloc();
  new_body->iterators = osl_strings_clone(body->iterators);
  char *expr = osl_strings_sprint(body->expression);
  expr = reprep(expr,
      alternative->value.name_to_swap,
      alternative->value.alt.function_to_swap);
  fprintf(stderr, "SWAPPED\n%s\n", expr);
  new_body->expression = osl_strings_encapsulate(expr);
  return new_body;
}

void acr_cloog_init_scop_to_match_alternatives(
    struct acr_runtime_data *data) {

  osl_statement_p statement = data->osl_relation->statement;
  osl_body_p initial_body;
  while (statement) {
    initial_body = NULL;
    for (size_t i = 0; i < data->num_alternatives; ++i) {
      if (data->alternatives[i].type == acr_runtime_alternative_function) {
        if (!initial_body)
          initial_body = osl_generic_lookup(statement->extension, OSL_URI_BODY);
        osl_body_p swapped_fun = _acr_body_copy_and_set_alternative_function(
            initial_body,
            &data->alternatives[i]);
        osl_statement_p new_statement = osl_statement_malloc();
        new_statement->scattering = osl_relation_clone(statement->scattering);
        new_statement->access = osl_relation_list_clone(statement->access);
        new_statement->domain = osl_relation_clone(statement->domain);
        osl_generic_add(&new_statement->extension,
            osl_generic_shell(swapped_fun, osl_body_interface()));
        new_statement->next = statement->next;
        statement->next = new_statement;
        statement = new_statement;
      }
    }
    statement = statement->next;
  }
}

void acr_cloog_get_rid_of_parameter(
    struct acr_runtime_data *data_info,
    unsigned int parameter_id,
    long int value) {
  // Context
  data_info->context =
    isl_set_remove_dims(data_info->context, isl_dim_param,
        0, 1);
  // Scattering
  for (size_t i = 0; i < data_info->num_statements; ++i) {
    data_info->statement_maps[i] =
      isl_map_remove_dims(data_info->statement_maps[i], isl_dim_param,
          0, 1);
  }
  // Domains
  for (size_t i = 0; i < data_info->num_alternatives; ++i) {
    struct runtime_alternative *alt = &data_info->alternatives[i];
    for (size_t j = 0; j < data_info->num_statements; ++j) {
      intmax_t parameter_val;
      if (alt->type == acr_runtime_alternative_parameter &&
          alt->value.alt.parameter.parameter_id == parameter_id) {
        parameter_val = alt->value.alt.parameter.parameter_value;
      } else {
        parameter_val = value;
      }
      isl_ctx *ctx = isl_set_get_ctx(alt->restricted_domains[j]);
      isl_val *val = isl_val_int_from_si(ctx, parameter_val);
      isl_local_space *lspace =
        isl_local_space_from_space(
            isl_set_get_space(alt->restricted_domains[j]));
      isl_constraint *constraint = isl_constraint_alloc_equality(lspace);
      constraint = isl_constraint_set_constant_val(constraint, val);
      constraint = isl_constraint_set_coefficient_si(constraint, isl_dim_param,
          0, -1);
      alt->restricted_domains[j] =
        isl_set_add_constraint(alt->restricted_domains[j], constraint);
      alt->restricted_domains[j] =
        isl_set_project_out(alt->restricted_domains[j], isl_dim_param,
            0, 1);
    }
  }
}

static isl_set** acr_isl_set_from_monitor(
    const struct acr_runtime_data *data_info,
    const unsigned char*data) {

  isl_set **sets = malloc(data_info->num_alternatives * sizeof(*sets));
  for (size_t i = 0; i < data_info->num_alternatives; ++i) {
    sets[i] = isl_set_copy(data_info->empty_monitor_set);
  }

  for(size_t i = 0; i < data_info->monitor_total_size; ++i) {
    struct runtime_alternative *alternative = data_info->alternative_from_val(data[i]);
    assert(alternative != NULL);

    sets[alternative->alternative_number] =
      isl_set_union(sets[alternative->alternative_number],
          isl_set_copy(data_info->tiles_domains[i]));
  }
  return sets;
}

void acr_cloog_generate_alternative_code_from_input(
    FILE* output,
    struct acr_runtime_data *data_info,
    const unsigned char *data) {

  CloogUnionDomain *new_udomain = cloog_union_domain_alloc(0);

  for (size_t i = 0; i < data_info->num_statements; ++i) {
    CloogDomain *cloog_domain;
    CloogScattering *cloog_scatt;

    new_udomain = cloog_union_domain_add_domain(new_udomain, NULL,
        (CloogDomain*)data_info->alternatives[0].restricted_domains[i],
        (CloogScattering*) data_info->statement_maps[i], NULL);
    CloogNamedDomainList * pragma_parameter_domain = new_udomain->domain;
    isl_map *statement_map_copy = isl_map_copy(data_info->statement_maps[i]);

    isl_set **alternative_domains = acr_isl_set_from_monitor(
        data_info, data);
    isl_set *alternative_set = NULL;
    const unsigned int num_dims = data_info->dimensions_per_statements[i];
    for (size_t j = 0; j < data_info->num_alternatives; ++j) {

      for(unsigned int k = 0; k < num_dims; ++k) {
        switch (data_info->statement_dimension_types[i][k]) {
          case acr_dimension_type_bound_to_alternative:
          case acr_dimension_type_free_dim:
            alternative_domains[j] =
              isl_set_insert_dims(alternative_domains[j], isl_dim_set, k, 1);
            break;
          case acr_dimension_type_bound_to_monitor:
            break;
          default:
            break;
        }
      }

      isl_set *alternative_real_domain =
        isl_set_intersect(alternative_domains[j],
            isl_set_copy(data_info->alternatives[j].restricted_domains[i]));
      switch (data_info->alternatives[j].type) {
        case acr_runtime_alternative_parameter:
          if (alternative_set)
            alternative_set = isl_set_union(alternative_set,
                alternative_real_domain);
          else
            alternative_set = alternative_real_domain;
          break;

        case acr_runtime_alternative_function:
          cloog_domain = cloog_domain_from_isl_set(alternative_real_domain);
          cloog_scatt = cloog_scattering_from_isl_map(isl_map_copy(statement_map_copy));
          new_udomain = cloog_union_domain_add_domain(new_udomain, NULL,
              cloog_domain, cloog_scatt, NULL);
          break;
      }
    }

    cloog_domain = cloog_domain_from_isl_set(alternative_set);
    cloog_scatt = cloog_scattering_from_isl_map(statement_map_copy);
    pragma_parameter_domain->scattering = cloog_scatt;
    pragma_parameter_domain->domain = cloog_domain;

    free(alternative_domains);
  }

  CloogOptions *cloog_option = cloog_options_malloc(data_info->state);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->scop = data_info->osl_relation;
  cloog_option->otl = 1;
  cloog_option->language = 0;
  CloogDomain *context =
    cloog_domain_from_isl_set(isl_set_copy(data_info->context));
  CloogProgram *cloog_program = cloog_program_alloc(context,
      new_udomain, cloog_option);
  cloog_program = cloog_program_generate(cloog_program, cloog_option);

  cloog_program_pprint(output, cloog_program, cloog_option);

  cloog_option->openscop = 0;
  cloog_option->scop = NULL;
  cloog_program_free(cloog_program);
  free(cloog_option);
}
