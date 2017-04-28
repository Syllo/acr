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

#include <acr/acr_runtime_code_generation.h>

#include <string.h>
#include <inttypes.h>

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
  /*fprintf(stderr, "SWAPPED\n%s\n", expr);*/
  new_body->expression = osl_strings_encapsulate(expr);
  return new_body;
}

void acr_gencode_init_scop_to_match_alternatives(
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
  for (size_t k = 0; k < data_info->num_codegen_threads; ++k) {
    // Context
    data_info->context[k] =
      isl_set_remove_dims(data_info->context[k], isl_dim_param,
          0, 1);
    // Scattering
    for (size_t i = 0; i < data_info->num_statements; ++i) {
      data_info->statement_maps[k][i] =
        isl_map_remove_dims(data_info->statement_maps[k][i], isl_dim_param,
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
        isl_ctx *ctx = isl_set_get_ctx(alt->restricted_domains[k][j]);
        isl_val *val = isl_val_int_from_si(ctx, parameter_val);
        isl_local_space *lspace =
          isl_local_space_from_space(
              isl_set_get_space(alt->restricted_domains[k][j]));
        isl_constraint *constraint = isl_constraint_alloc_equality(lspace);
        constraint = isl_constraint_set_constant_val(constraint, val);
        constraint = isl_constraint_set_coefficient_si(constraint, isl_dim_param,
            0, -1);
        alt->restricted_domains[k][j] =
          isl_set_add_constraint(alt->restricted_domains[k][j], constraint);
        alt->restricted_domains[k][j] =
          isl_set_project_out(alt->restricted_domains[k][j], isl_dim_param,
              0, 1);
      }
    }
  }
}

static void acr_isl_set_from_monitor(
    const struct acr_runtime_data *data_info,
    const unsigned char*data,
    size_t thread_num,
    isl_set **sets) {

  for (size_t i = 0; i < data_info->num_alternatives; ++i) {
    sets[i] = isl_set_copy(data_info->empty_monitor_set[thread_num]);
  }

  for(size_t i = 0; i < data_info->monitor_total_size; ++i) {
    struct runtime_alternative *alternative = data_info->alternative_from_val(data[i]);
    assert(alternative != NULL);

    sets[alternative->alternative_number] =
      isl_set_union(sets[alternative->alternative_number],
          isl_set_copy(data_info->tiles_domains[thread_num][i]));
  }
}

void acr_cloog_generate_alternative_code_from_input(
    FILE* output,
    const struct acr_runtime_data *data_info,
    const unsigned char *data,
    size_t thread_num,
    isl_set **temporary_alt_domain) {

  CloogUnionDomain *new_udomain = cloog_union_domain_alloc(0);

  acr_isl_set_from_monitor(
      data_info, data, thread_num, temporary_alt_domain);

  /*isl_printer *splinter = isl_printer_to_file(isl_set_get_ctx(*temporary_alt_domain), stderr);*/
  /*splinter = isl_printer_set_output_format(splinter, ISL_FORMAT_EXT_POLYLIB);*/
  /*splinter = isl_printer_print_set(splinter, (data_info->context[thread_num]));*/

  CloogNamedDomainList *current_domain = NULL;
  for (size_t i = 0; i < data_info->num_statements; ++i) {
    CloogDomain *cloog_domain;
    CloogScattering *cloog_scatt;

    new_udomain = cloog_union_domain_add_domain(new_udomain, NULL,
        (CloogDomain*)data_info->alternatives[0].restricted_domains[thread_num][i],
        (CloogScattering*) data_info->statement_maps[thread_num][i], NULL);
    if (current_domain)
      current_domain = current_domain->next;
    else
      current_domain = new_udomain->domain;
    CloogNamedDomainList *pragma_parameter_domain = current_domain;
    isl_map *statement_map_copy =
      isl_map_copy(data_info->statement_maps[thread_num][i]);

    isl_set *alternative_set = NULL;
    const unsigned int num_dims = data_info->dimensions_per_statements[i];
    for (size_t j = 0; j < data_info->num_alternatives; ++j) {
      isl_set *adjusted_set;
      if (i == data_info->num_statements - 1)
        adjusted_set = temporary_alt_domain[j];
      else
        adjusted_set = isl_set_copy(temporary_alt_domain[j]);
      unsigned int num_bound_to_monitor = 0u;
      int first_monitor_dimension = -1;
      for(unsigned int k = 0; k < num_dims; ++k) {
        switch (data_info->statement_dimension_types[i][k]) {
          case acr_dimension_type_bound_to_alternative:
          case acr_dimension_type_free_dim:
             adjusted_set =
              isl_set_insert_dims(adjusted_set,
                  isl_dim_set, k, 1);
            break;
          case acr_dimension_type_bound_to_monitor:
            if (first_monitor_dimension == -1)
              first_monitor_dimension = (int) k;
            num_bound_to_monitor += 1;
            break;
          default:
            break;
        }
      }
      const unsigned int num_monitor_dims = data_info->num_monitor_dims;
      if (num_bound_to_monitor != num_monitor_dims) {
        if (first_monitor_dimension == -1)
          first_monitor_dimension = (int) num_dims;

        unsigned int position = first_monitor_dimension == -1 ? num_dims :
          (unsigned int)first_monitor_dimension + num_bound_to_monitor;
        unsigned int num_dims_to_remove = num_monitor_dims - num_bound_to_monitor;
        isl_set_remove_dims(adjusted_set, isl_dim_set, position, num_dims_to_remove);
      }

      /*fprintf(stderr, "isl_set\n\n");*/
      /*adjusted_set = isl_set_coalesce(adjusted_set);*/
      /*isl_set_print_internal(adjusted_set, stderr, 0);*/
      /*fprintf(stderr, "Restriction\n\n");*/
      /*isl_set_print_internal(data_info->alternatives[j].restricted_domains[thread_num][i], stderr, 0);*/

      isl_set *alternative_real_domain =
        isl_set_intersect(adjusted_set,
            isl_set_copy(data_info->alternatives[j].restricted_domains[thread_num][i]));
      /*fprintf(stderr, "Real domain\n\n");*/
      /*isl_set_print_internal(alternative_real_domain, stderr, 0);*/
      switch (data_info->alternatives[j].type) {
        case acr_runtime_alternative_full_computation:
        case acr_runtime_alternative_zero_computation:
        case acr_runtime_alternative_corner_computation:
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
          current_domain = current_domain->next;
          break;
      }
    }

    if (alternative_set) {
      cloog_domain = cloog_domain_from_isl_set(alternative_set);
      cloog_scatt = cloog_scattering_from_isl_map(statement_map_copy);
    } else {
      cloog_domain = cloog_domain_from_isl_set(isl_set_copy(data_info->empty_monitor_set[thread_num]));
      cloog_scatt = cloog_scattering_from_isl_map(statement_map_copy);
    }
    pragma_parameter_domain->scattering = cloog_scatt;
    pragma_parameter_domain->domain = cloog_domain;

    /*splinter = isl_printer_print_set(splinter, ((isl_set*)cloog_domain));*/
    /*splinter = isl_printer_print_map(splinter, isl_map_coalesce((isl_map*) cloog_scatt));*/
  }
  /*isl_printer_free(splinter);*/

  CloogOptions *cloog_option = cloog_options_malloc(data_info->state[thread_num]);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->scop = data_info->osl_relation;
  cloog_option->esp = 1;
  cloog_option->strides = 1;
  CloogDomain *context =
    cloog_domain_from_isl_set(isl_set_copy(data_info->context[thread_num]));
  CloogProgram *cloog_program = cloog_program_alloc(context,
      new_udomain, cloog_option);
  cloog_program = cloog_program_generate(cloog_program, cloog_option);

  cloog_program_pprint(output, cloog_program, cloog_option);

  cloog_option->openscop = 0;
  cloog_option->scop = NULL;
  cloog_program_free(cloog_program);
  free(cloog_option);
}

isl_map* isl_map_from_cloog_scattering(CloogScattering *scat);

void acr_runtime_apply_tiling(
    size_t tiling_size,
    size_t dimension_start_tiling,
    size_t num_dimensions_to_tile,
    CloogUnionDomain *ud) {
  CloogNamedDomainList *statement = ud->domain;
  while (statement) {
    isl_set *domain =
      isl_set_from_cloog_domain(statement->domain);
    isl_map *scattering =
      isl_map_from_cloog_scattering(statement->scattering);

    const unsigned long num_dims_in_statement = isl_set_n_dim(domain);
    const unsigned int num_tiling_dim_in_statement =
      num_dims_in_statement < dimension_start_tiling ? 0 :
      (unsigned int) (num_dims_in_statement - dimension_start_tiling);

    if (num_tiling_dim_in_statement == 0) {
      statement = statement->next;
      continue;
    }

    const unsigned int scat_dim_start =
        2*(unsigned int)dimension_start_tiling+1;
    const unsigned int dim_start =
      scat_dim_start + (unsigned int)num_dimensions_to_tile*2;

    scattering = isl_map_insert_dims(scattering, isl_dim_out,
        scat_dim_start,
        (unsigned int)num_dimensions_to_tile * 2);
    for (unsigned int i = 0; i < num_tiling_dim_in_statement; ++i) {
      isl_val *tiling = isl_val_int_from_ui(isl_set_get_ctx(domain), tiling_size);
      isl_space *space = isl_map_get_space(scattering);
      isl_local_space *lspace = isl_local_space_from_space(space);
      isl_constraint *c1 = isl_constraint_alloc_equality(isl_local_space_copy(lspace));
      isl_constraint *c2 = isl_constraint_alloc_inequality(isl_local_space_copy(lspace));
      isl_constraint *c3 = isl_constraint_alloc_inequality(lspace);

      c1 = isl_constraint_set_coefficient_si(c1, isl_dim_out, (int)scat_dim_start+(int)i*2+1, 1);
      c2 = isl_constraint_set_coefficient_val(c2, isl_dim_out,
          (int)scat_dim_start+(int)i*2, isl_val_neg(isl_val_copy(tiling)));
      c2 = isl_constraint_set_coefficient_si(c2, isl_dim_out,
          (int)dim_start+(int)i*2, 1);
      c3 = isl_constraint_set_coefficient_val(c3, isl_dim_out,
          (int)scat_dim_start+(int)i*2, (isl_val_copy(tiling)));
      c3 = isl_constraint_set_coefficient_si(c3, isl_dim_out,
          (int)dim_start+(int)i*2, -1);
      c3 = isl_constraint_set_constant_val(c3, isl_val_sub_ui(tiling, 1));
      scattering = isl_map_add_constraint(scattering, c1);
      scattering = isl_map_add_constraint(scattering, c2);
      scattering = isl_map_add_constraint(scattering, c3);
    }
    for (unsigned long i = num_tiling_dim_in_statement; i < num_dimensions_to_tile; ++i) {
      isl_space *space = isl_map_get_space(scattering);
      isl_local_space *lspace = isl_local_space_from_space(space);
      isl_constraint *c1 = isl_constraint_alloc_equality(isl_local_space_copy(lspace));
      isl_constraint *c2 = isl_constraint_alloc_equality(lspace);
      c1 = isl_constraint_set_coefficient_si(c1, isl_dim_out, (int)scat_dim_start+(int)i*2+1, 1);
      c2 = isl_constraint_set_coefficient_si(c2, isl_dim_out, (int)scat_dim_start+(int)i*2, 1);
      scattering = isl_map_add_constraint(scattering, c1);
      scattering = isl_map_add_constraint(scattering, c2);
    }

    statement->domain = cloog_domain_from_isl_set(domain);
    statement->scattering = cloog_scattering_from_isl_map(scattering);

    statement = statement->next;
  }
}

void acr_runtime_apply_reduction_function(
    CloogDomain *context,
    CloogUnionDomain const *ud,
    size_t first_monitor_dimension,
    size_t number_of_monitoring_dimensions,
    CloogUnionDomain **new_ud,
    CloogDomain **new_context,
    osl_scop_p *new_scop,
    const char *const scan_corpse,
    char const*const iterators[],
    bool keep_parameters) {

  isl_set *context_isl = isl_set_copy(isl_set_from_cloog_domain(context));

  CloogNamedDomainList *statement = ud->domain;
  while (statement) {
    if (isl_set_n_dim(isl_set_from_cloog_domain(statement->domain)) >=
        first_monitor_dimension + number_of_monitoring_dimensions)
      break;
    statement = statement->next;
  }

  *new_ud = cloog_union_domain_alloc(0);

  isl_set *domain =
    isl_set_copy(isl_set_from_cloog_domain(statement->domain));
  isl_map *scatt =
    isl_map_copy(isl_map_from_cloog_scattering(statement->scattering));

  if (first_monitor_dimension > 0) {
    domain = isl_set_project_out(domain, isl_dim_out, 0,
        (unsigned int)first_monitor_dimension);
    scatt = isl_map_project_out(scatt, isl_dim_out, 0,
        (unsigned int)first_monitor_dimension*2);
    scatt = isl_map_project_out(scatt, isl_dim_in, 0,
        (unsigned int)first_monitor_dimension);
  }
  if (number_of_monitoring_dimensions < isl_set_n_dim(domain)) {
    domain = isl_set_project_out(domain, isl_dim_out,
        (unsigned int)number_of_monitoring_dimensions,
        isl_set_n_dim(domain) - (unsigned int)number_of_monitoring_dimensions);
    scatt = isl_map_project_out(scatt, isl_dim_in,
        (unsigned int)number_of_monitoring_dimensions,
        isl_set_n_dim(domain) - (unsigned int)number_of_monitoring_dimensions);
    scatt = isl_map_project_out(scatt, isl_dim_in,
        (unsigned int)number_of_monitoring_dimensions*2,
        2*(isl_set_n_dim(domain) - (unsigned int)number_of_monitoring_dimensions));
  }
  isl_val *one = isl_val_one(isl_set_get_ctx(domain));
  for (size_t i = 0; i < 2*number_of_monitoring_dimensions; ++i) {
    isl_constraint *c = isl_constraint_alloc_equality(
        isl_local_space_from_space(isl_map_get_space(scatt)));
    c = isl_constraint_set_coefficient_val(c, isl_dim_out, (int)i*2, isl_val_copy(one));
    scatt = isl_map_drop_constraints_involving_dims(scatt, isl_dim_out,
        (unsigned int)i*2, 1);
    scatt = isl_map_add_constraint(scatt, c);
  }
  isl_val_free(one);

  if (!keep_parameters) {
    domain = isl_set_project_out(domain, isl_dim_param, 0,
        isl_set_n_param(domain));
    scatt = isl_map_project_out(scatt, isl_dim_param, 0, isl_map_n_param(scatt));
    context_isl = isl_set_project_out(context_isl, isl_dim_param, 0,
        isl_set_n_param(context_isl));
  }

  *new_ud = cloog_union_domain_add_domain(*new_ud,
      NULL, cloog_domain_from_isl_set(domain),
      cloog_scattering_from_isl_map(scatt), NULL);

  osl_scop_p scop = osl_scop_malloc();
  scop->registry = osl_interface_get_default_registry();
  scop->statement = osl_statement_malloc();
  osl_body_p body = osl_body_malloc();
  osl_strings_p osl_iterators = osl_strings_malloc();
  osl_strings_p corpse = osl_strings_malloc();
  size_t i = 0;
  while (iterators[i] != NULL) {
    osl_strings_add(osl_iterators, iterators[i]);
    ++i;
  }
  osl_strings_add(corpse, scan_corpse);
  body->iterators = osl_iterators;
  body->expression = corpse;
  osl_generic_add(&scop->statement->extension, osl_generic_shell(body, osl_body_interface()));
  *new_scop = scop;

  *new_context = cloog_domain_from_isl_set(context_isl);
}
