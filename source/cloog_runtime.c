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
      named_domain = data->cloog_input->ud->domain;
      for (unsigned long j = 0; j < num_domains; ++j) {
        CloogDomain *domain = named_domain->domain;
        isl_set *isl_temp_set = isl_set_from_cloog_domain(domain);
        unsigned long num_dim = isl_set_n_dim(isl_temp_set);
        unsigned long num_param = isl_set_n_param(isl_temp_set);
      }
    }
  }

}

void acr_cloog_generate_alternative_code_from_input(
    FILE* output,
    struct acr_runtime_data *data) {

  CloogNamedDomainList *domain = data->cloog_input->ud->domain;
  CloogDomain *context = data->cloog_input->context;
  isl_set *isl_context = isl_set_from_cloog_domain(context);
  unsigned long num_param = isl_set_n_param(isl_context);

  cloog_domain_print_structure(stderr, domain->domain, 0, "A");

  CloogUnionDomain *new_udomain = cloog_union_domain_alloc(num_param);
  for (unsigned long i = 0; i < data->num_alternatives; ++i) {
    CloogDomain *previous_domain = cloog_domain_copy(domain->domain);
    isl_set *isl_temp_set = isl_set_from_cloog_domain(previous_domain);
    unsigned long num_dim = isl_set_n_dim(isl_temp_set);
    switch (data->alternatives[i].type) {
      case acr_runtime_alternative_parameter:
        break;
      case acr_runtime_alternative_function:
        break;
    }
  }
}
