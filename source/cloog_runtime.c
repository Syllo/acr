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

char* acr_cloog_generate_alternative_code_from_input(
    CloogState *state,
    CloogInput *cloog_input,
    unsigned long num_alternatives,
    struct runtime_alternative *alternatives,
    isl_set **sets) {
  CloogNamedDomainList *domain = cloog_input->ud->domain;
  CloogDomain *context = cloog_input->context;
  isl_set *isl_context = isl_set_from_cloog_domain(context);
  unsigned long num_param = isl_set_n_param(isl_context);
  CloogUnionDomain *new_udomain = cloog_union_domain_alloc(num_param);
  for (unsigned long i = 0; i < num_alternatives; ++i) {
    CloogDomain *temp = cloog_domain_from_isl_set(sets[i]);
    CloogDomain *previous_domain = cloog_domain_copy(domain->domain);
    isl_set *isl_temp_set = isl_set_from_cloog_domain(previous_domain);
    unsigned long num_dim = isl_set_n_dim(isl_temp_set);
  }
}
