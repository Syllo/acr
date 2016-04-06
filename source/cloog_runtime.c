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
    for (unsigned long i = 0; i < data->num_alternatives; ++i) {
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
  CloogNamedDomainList *named_domain = data->cloog_input->ud->domain;
  unsigned long num_domains = 0ul;
  while(named_domain) {
    ++num_domains;
    named_domain = named_domain->next;
  }

  for (unsigned long i = 0; i < data->num_alternatives; ++i) {
    struct runtime_alternative *alt = &data->alternatives[i];
    if (alt->type == acr_runtime_alternative_parameter) {
      alt->value.alt.parameter.parameter_constraints =
        malloc(num_domains *
            sizeof(*alt->value.alt.parameter.parameter_constraints));
      alt->value.alt.parameter.num_domains = num_domains;
      named_domain = data->cloog_input->ud->domain;
      for (unsigned long j = 0; j < num_domains; ++j) {
        CloogDomain *domain = named_domain->domain;
        isl_set *isl_temp_set = isl_set_from_cloog_domain(domain);
        unsigned long num_dim = isl_set_n_dim(isl_temp_set);
        unsigned long num_param = isl_set_n_param(isl_temp_set);
        isl_ctx *ctx = isl_set_get_ctx(isl_temp_set);
        alt->value.alt.parameter.parameter_constraints[j] =
          acr_isl_set_from_alternative_parameter_construct(ctx,
              num_param, num_dim, alt);
        named_domain = named_domain->next;
      }
    }
  }
}

isl_map* isl_map_from_cloog_scattering(CloogScattering *scat);

void acr_cloog_generate_alternative_code_from_input(
    FILE* output,
    struct acr_runtime_data *data_info,
    const unsigned char *data) {

  CloogNamedDomainList *domain_list = data_info->cloog_input->ud->domain;
  CloogDomain *context = cloog_domain_copy(data_info->cloog_input->context);
  isl_set *isl_context = isl_set_from_cloog_domain(context);
  unsigned long num_param = isl_set_n_param(isl_context);
  CloogUnionDomain *new_udomain = cloog_union_domain_alloc(num_param);

  const osl_generic_p osl_parameters = data_info->osl_relation->parameters;
  if (osl_generic_has_URI(osl_parameters, OSL_URI_STRINGS)) {
    for (size_t i = 0; i < osl_strings_size(osl_parameters->data); i++) {
      new_udomain = cloog_union_domain_set_name(new_udomain, CLOOG_PARAM, i,
          ((osl_strings_p)(osl_parameters->data))->string[i]);
    }
  }
  unsigned long statement_num = 0;
  while(domain_list) {
    CloogDomain *cloog_domain;
    CloogScattering *cloog_scatt;
    new_udomain = cloog_union_domain_add_domain(new_udomain, NULL,
        domain_list->domain, domain_list->scattering, NULL);
    CloogNamedDomainList * pragma_parameter_domain = new_udomain->domain;
    isl_map *statement_map_copy = isl_map_copy(isl_map_from_cloog_scattering(
          domain_list->scattering));
    isl_set *statement_set_copy =
      isl_set_copy(isl_set_from_cloog_domain(domain_list->domain));
    isl_ctx *ctx = isl_set_get_ctx(statement_set_copy);
    isl_set **alternative_domains = acr_isl_set_from_monitor(ctx,
        data, data_info->num_alternatives, data_info->num_parameters,
        data_info->num_monitor_dims, data_info->monitor_dim_max,
        data_info->monitor_total_size, data_info->grid_size,
        data_info->alternative_from_val);

    isl_set *alternative_set = NULL;
    const unsigned long num_dims = data_info->dimensions_per_statements[statement_num];
    for (unsigned long i = 0; i < data_info->num_alternatives; ++i) {
      for(unsigned long j = 0; j < num_dims; ++j) {
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
          alternative_domains[i] = isl_set_intersect(alternative_domains[i],
              isl_set_copy(
                *data_info->alternatives[i].value.alt.parameter.parameter_constraints));
          if (alternative_set)
            alternative_set = isl_set_union(alternative_set,
                isl_set_intersect(alternative_domains[i],
                  isl_set_copy(statement_set_copy)));
          else
            alternative_set = isl_set_intersect(alternative_domains[i],
                isl_set_copy(statement_set_copy));
          alternative_set = isl_set_eliminate(
              alternative_set, isl_dim_param,
              data_info->alternatives[i].value.alt.parameter.parameter_position,
              1);
          break;

        case acr_runtime_alternative_function:
          alternative_domains[i] = isl_set_intersect(alternative_domains[i],
              isl_set_copy(statement_set_copy));
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
    isl_set_free(statement_set_copy);
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
