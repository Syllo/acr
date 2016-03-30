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
#include <acr/isl_runtime.h>

#include <cloog/domain.h>
#include <cloog/isl/domain.h>
#include <cloog/isl/backend.h>
#include <isl/map.h>

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
    const unsigned char *data,
    struct runtime_alternative*
        (*get_alternative_from_val)(unsigned char data)) {

  CloogNamedDomainList *domain_list = data_info->cloog_input->ud->domain;
  CloogDomain *context = data_info->cloog_input->context;
  isl_set *isl_context = isl_set_from_cloog_domain(context);
  unsigned long num_param = isl_set_n_param(isl_context);
  CloogUnionDomain *new_udomain = cloog_union_domain_alloc(num_param);

  if (osl_generic_has_URI(data_info->osl_relation->parameters, OSL_URI_STRINGS)) {
    for (size_t i = 0; i < osl_strings_size(data_info->osl_relation->parameters->data); i++) {
      new_udomain = cloog_union_domain_set_name(new_udomain, CLOOG_PARAM, i,
        ((osl_strings_p)(data_info->osl_relation->parameters->data))->string[i]);
    }
  }

  unsigned long statement_num = 0;
  while(domain_list) {
    isl_map *original_scattering =
      isl_map_from_cloog_scattering(domain_list->scattering);
    isl_map *statement_map_copy = isl_map_copy(original_scattering);
    isl_set *statement_set_copy =
      isl_set_copy(isl_set_from_cloog_domain(domain_list->domain));
    isl_ctx *ctx = isl_set_get_ctx(statement_set_copy);
    isl_set **alternative_domains = acr_isl_set_from_monitor(ctx,
        data, data_info->num_alternatives, data_info->num_parameters,
        data_info->num_monitor_dims, data_info->monitor_dim_max,
        data_info->monitor_total_size, data_info->grid_size,
        get_alternative_from_val);
    acr_isl_set_add_missing_dim_in_statement(data_info, statement_num,
        alternative_domains);

    isl_set *alternative_set = NULL;
    for (unsigned long i = 0; i < data_info->num_alternatives; ++i) {
      switch (data_info->alternatives[i].type) {
        case acr_runtime_alternative_parameter:
          alternative_domains[i] = isl_set_intersect_params(alternative_domains[i],
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
          isl_set_free(alternative_domains[i]);
          break;
      }
    }

    CloogDomain *cloog_domain = cloog_domain_from_isl_set(alternative_set);
    CloogScattering *cloog_scatt = cloog_scattering_from_isl_map(statement_map_copy);
    new_udomain = cloog_union_domain_add_domain(new_udomain, NULL,
        cloog_domain, cloog_scatt, NULL);

    domain_list = domain_list->next;
    statement_num += 1;
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

}
