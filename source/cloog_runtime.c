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

#include <strings.h>

#include <acr/isl_runtime.h>

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

void acr_cloog_init_alternative_constraint_from_cloog_union_domain(
    struct acr_runtime_data *data) {
  const unsigned long num_statements = data->num_statements;
  const size_t num_alternatives = data->num_alternatives;

  for (size_t i = 0; i < num_alternatives; ++i) {
    struct runtime_alternative *alt = &data->alternatives[i];
    alt->restricted_domains =
      malloc(num_statements * sizeof(*alt->restricted_domains));
    CloogNamedDomainList *named_domain = data->cloog_input->ud->domain;
    for (size_t j = 0; j < num_statements; ++j) {
      CloogDomain *domain = named_domain->domain;
      isl_set *isl_temp_set = isl_set_from_cloog_domain(domain);
      alt->restricted_domains[j] = isl_set_copy(isl_temp_set);
      // Add alternative constraint
      if (alt->type == acr_alternative_parameter) {
        isl_ctx *ctx = isl_set_get_ctx(isl_temp_set);
        isl_val *value =
          isl_val_int_from_si(ctx, alt->value.alt.parameter.parameter_value);
        isl_local_space *local_space =
          isl_local_space_from_space(isl_set_get_space(isl_temp_set));
        isl_constraint *constraint = isl_constraint_alloc_equality(local_space);
        constraint = isl_constraint_set_constant_val(constraint, value);
        constraint = isl_constraint_set_coefficient_si(constraint,
            isl_dim_param, alt->value.alt.parameter.parameter_position, -1);
        /*isl_set_add_constraint()*/
      }
    }
  }
}


isl_map* isl_map_from_cloog_scattering(CloogScattering *scat);

void acr_cloog_get_rid_of_parameter(
    struct acr_runtime_data *data_info,
    unsigned int parameter,
    long int value) {
  CloogNamedDomainList *domain_list = data_info->cloog_input->ud->domain;

  isl_set *context = isl_set_from_cloog_domain(
      data_info->cloog_input->context);
  isl_ctx *ctx = isl_set_get_ctx(context);
  isl_val *parameter_value = isl_val_int_from_si(ctx, value);
  isl_local_space *local_space;
  isl_constraint *fixed_parameter;
  while(domain_list) {
    isl_map *statement_map = isl_map_from_cloog_scattering(
          domain_list->scattering);
    isl_set *statement_set = isl_set_from_cloog_domain(
          domain_list->domain);
    local_space =
      isl_local_space_from_space(isl_set_get_space(statement_set));
    fixed_parameter = isl_constraint_alloc_equality(local_space);
    fixed_parameter = isl_constraint_set_constant_val(fixed_parameter,
        isl_val_copy(parameter_value));
    fixed_parameter = isl_constraint_set_coefficient_si(fixed_parameter,
        isl_dim_param, (int)parameter, -1);
    statement_set = isl_set_add_constraint(statement_set, fixed_parameter);
    statement_set = isl_set_project_out(statement_set, isl_dim_param,
        parameter, 1);
    statement_map = isl_map_project_out(statement_map, isl_dim_param,
        parameter, 1);
    domain_list->domain = cloog_domain_from_isl_set(statement_set);
    domain_list->scattering = cloog_scattering_from_isl_map(statement_map);

    domain_list = domain_list->next;
  }
  local_space =
    isl_local_space_from_space(isl_set_get_space(context));
  fixed_parameter = isl_constraint_alloc_equality(local_space);
  fixed_parameter =
    isl_constraint_set_constant_val(fixed_parameter, parameter_value);
  fixed_parameter = isl_constraint_set_coefficient_si(fixed_parameter,
      isl_dim_param, (int)parameter, -1);
  context = isl_set_add_constraint(context, fixed_parameter);
  context = isl_set_project_out(context, isl_dim_param, parameter, 1);
  data_info->cloog_input->context = cloog_domain_from_isl_set(context);
  free(data_info->cloog_input->ud->name[CLOOG_PARAM][parameter]);
  for (int i = (int)parameter+1; i < data_info->cloog_input->ud->n_name[CLOOG_PARAM]; ++i) {
    data_info->cloog_input->ud->name[CLOOG_PARAM][i] =
      data_info->cloog_input->ud->name[CLOOG_PARAM][i-1];
  }
  data_info->cloog_input->ud->n_name[CLOOG_PARAM] -= 1;

}


void acr_cloog_generate_alternative_code_from_input(
    FILE* output,
    struct acr_runtime_data *data_info,
    const unsigned char *data) {

  CloogNamedDomainList *domain_list = data_info->cloog_input->ud->domain;
  CloogDomain *context = cloog_domain_copy(data_info->cloog_input->context);
  isl_set *isl_context = isl_set_from_cloog_domain(context);
  unsigned int num_param = isl_set_n_param(isl_context);
  CloogUnionDomain *new_udomain = cloog_union_domain_alloc((int)num_param);

  size_t statement_num = 0;
  while(domain_list) {
    CloogDomain *cloog_domain;
    CloogScattering *cloog_scatt;
    new_udomain = cloog_union_domain_add_domain(new_udomain, NULL,
        domain_list->domain, domain_list->scattering, NULL);
    CloogNamedDomainList * pragma_parameter_domain = new_udomain->domain;
    isl_map *statement_map_copy = isl_map_copy(isl_map_from_cloog_scattering(
          domain_list->scattering));
    isl_ctx *ctx =
      isl_set_get_ctx(isl_set_from_cloog_domain(domain_list->domain));
    isl_set **alternative_domains = acr_isl_set_from_monitor(ctx,
        data, data_info->num_alternatives, data_info->num_parameters,
        data_info->num_monitor_dims, data_info->monitor_dim_max,
        data_info->monitor_total_size, data_info->grid_size,
        data_info->alternative_from_val);

    isl_set *alternative_set = NULL;
    const unsigned int num_dims = data_info->dimensions_per_statements[statement_num];
    for (size_t i = 0; i < data_info->num_alternatives; ++i) {
      for(unsigned int j = 0; j < num_dims; ++j) {
        switch (data_info->statement_dimension_types[statement_num][j]) {
          case acr_dimension_type_bound_to_alternative:
          case acr_dimension_type_free_dim:
            alternative_domains[i] =
              isl_set_insert_dims(alternative_domains[i], isl_dim_set, j, 1);
            break;
          case acr_dimension_type_bound_to_monitor:
            break;
          default:
            break;
        }
      }
      switch (data_info->alternatives[i].type) {
        case acr_runtime_alternative_parameter:
          if (alternative_set)
            alternative_set = isl_set_union(alternative_set,
                isl_set_intersect(alternative_domains[i],
                  isl_set_copy(data_info->alternatives[i].
                    restricted_domains[statement_num])));
          else
            alternative_set = isl_set_intersect(alternative_domains[i],
                isl_set_copy(data_info->alternatives[i].
                  restricted_domains[statement_num]));
          break;

        case acr_runtime_alternative_function:
          alternative_domains[i] = isl_set_intersect(alternative_domains[i],
              isl_set_copy(data_info->alternatives[i].
                restricted_domains[statement_num]));
          cloog_domain = cloog_domain_from_isl_set(alternative_domains[i]);
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

    domain_list = domain_list->next;
    statement_num += 1;
    free(alternative_domains);
  }

  CloogOptions *cloog_option = cloog_options_malloc(data_info->state);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->scop = data_info->osl_relation;
  cloog_option->otl = 1;
  cloog_option->language = 0;
  CloogProgram *cloog_program = cloog_program_alloc(context,
      new_udomain, cloog_option);
  cloog_program = cloog_program_generate(cloog_program, cloog_option);

  cloog_program_pprint(output, cloog_program, cloog_option);

  cloog_option->openscop = 0;
  cloog_option->scop = NULL;
  cloog_program_free(cloog_program);
  free(cloog_option);
}
