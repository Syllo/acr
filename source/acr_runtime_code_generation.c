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
  fprintf(stderr, "SWAPPED\n%s\n", expr);
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

      for(unsigned int k = 0; k < num_dims; ++k) {
        switch (data_info->statement_dimension_types[i][k]) {
          case acr_dimension_type_bound_to_alternative:
          case acr_dimension_type_free_dim:
            temporary_alt_domain[j] =
              isl_set_insert_dims(temporary_alt_domain[j],
                  isl_dim_set, k, 1);
            break;
          case acr_dimension_type_bound_to_monitor:
            break;
          default:
            break;
        }
      }

      isl_set *alternative_real_domain =
        isl_set_intersect(temporary_alt_domain[j],
            isl_set_copy(data_info->alternatives[j].restricted_domains[thread_num][i]));
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
          current_domain = current_domain->next;
          break;
      }
    }

    cloog_domain = cloog_domain_from_isl_set(alternative_set);
    cloog_scatt = cloog_scattering_from_isl_map(statement_map_copy);
    pragma_parameter_domain->scattering = cloog_scatt;
    pragma_parameter_domain->domain = cloog_domain;

  }

  CloogOptions *cloog_option = cloog_options_malloc(data_info->state[thread_num]);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->scop = data_info->osl_relation;
  cloog_option->otl = 1;
  cloog_option->language = 0;
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

void acr_cloog_get_rid_of_parameter_static(
    struct acr_runtime_data_static *data_info,
    unsigned int parameter_index,
    long int value) {

  isl_set *context = isl_set_from_cloog_domain(data_info->context);
  data_info->context =
    cloog_domain_from_isl_set(
        isl_set_remove_dims(context, isl_dim_param, parameter_index, 1));
  CloogNamedDomainList *statement_list = data_info->union_domain->domain;
  while (statement_list != NULL) {
    isl_map *scat = isl_map_from_cloog_scattering(statement_list->scattering);
    statement_list->scattering =
      cloog_scattering_from_isl_map(
          isl_map_remove_dims(scat, isl_dim_param, parameter_index, 1));
    /*fprintf(stderr, "Statement before\n");*/
    /*cloog_domain_print_constraints(stderr, statement_list->domain, 0);*/
    isl_set *domain = isl_set_from_cloog_domain(statement_list->domain);
    isl_ctx *ctx = isl_set_get_ctx(domain);
    isl_val *val = isl_val_int_from_si(ctx, value);
    isl_local_space *lspace =
      isl_local_space_from_space(
          isl_set_get_space(domain));
    isl_constraint *constraint = isl_constraint_alloc_equality(lspace);
    constraint = isl_constraint_set_constant_val(constraint, val);
    constraint = isl_constraint_set_coefficient_si(constraint, isl_dim_param,
        (int)parameter_index, -1);
    domain = isl_set_add_constraint(domain, constraint);
    domain = isl_set_project_out(domain, isl_dim_param, parameter_index, 1);
    statement_list->domain = cloog_domain_from_isl_set(domain);
    /*fprintf(stderr, "Statement after\n");*/
    /*cloog_domain_print_constraints(stderr, statement_list->domain, 0);*/
    statement_list = statement_list->next;
  }
}

static void acr_runtime_apply_tiling(
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

static void acr_runtime_apply_reduction_function(
    CloogDomain *context,
    CloogUnionDomain const *ud,
    size_t first_monitor_dimension,
    size_t number_of_monitoring_dimensions,
    CloogUnionDomain **new_ud,
    CloogDomain **new_context,
    osl_scop_p *new_scop,
    const char *const scan_corpse,
    char const*const iterators[]) {

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

  domain = isl_set_project_out(domain, isl_dim_param, 0,
      isl_set_n_param(domain));
  scatt = isl_map_project_out(scatt, isl_dim_param, 0, isl_map_n_param(scatt));
  context_isl = isl_set_project_out(context_isl, isl_dim_param, 0,
      isl_set_n_param(context_isl));

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

static void acr_print_tiling_library_preamble(FILE *out) {
  fprintf(out,
      "#include <stdlib.h>\n"
      "#include \"acr_required_functions.h\"\n"
      "static inline void const* __acr_tmp_to_function(size_t, unsigned char, void const*const*);\n");
}

static void acr_print_tiling_library_postamble(
    FILE *out,
    struct acr_runtime_data_static *static_data) {
  fprintf(out,
      "static inline void const* __acr_tmp_to_function(size_t tile_num,"
      " unsigned char li, void const*const*const all_functions) {\n"
      "  if (li > %zu)\n"
      "    return NULL;\n"
      "  else\n"
      "    return all_functions[%zu*li+tile_num];\n"
      "}\n", static_data->num_alternatives, static_data->num_fun_per_alt);
}

static inline CloogUnionDomain* acr_copy_cloog_ud(CloogUnionDomain const *ud) {
  CloogUnionDomain *ud_copy =
    cloog_union_domain_alloc(ud->n_name[CLOOG_PARAM]);
  CloogNamedDomainList *statement = ud->domain;
  while (statement) {
    ud_copy = cloog_union_domain_add_domain(ud_copy, NULL,
        (CloogDomain*)isl_set_copy((isl_set*)statement->domain),
        (CloogScattering*)isl_map_copy((isl_map*)statement->scattering),
        NULL);
    statement = statement->next;
  }
  return ud_copy;
}

static inline isl_val* get_val_dim(isl_set *set, unsigned int dimi) {
  isl_val *valdim = NULL;
  isl_basic_set_list *bsl = isl_set_get_basic_set_list(set);
  assert(isl_basic_set_list_n_basic_set(bsl) == 1);
  isl_basic_set *bs = isl_basic_set_list_get_basic_set(bsl, 0);
  isl_constraint_list *cl = isl_basic_set_get_constraint_list(bs);
  int size = isl_constraint_list_n_constraint(cl);
  for (int i = 0; i < size; ++i) {
    isl_constraint *c = isl_constraint_list_get_constraint(cl, i);
    if (isl_constraint_involves_dims(c, isl_dim_out, dimi, 1)) {
      valdim = isl_constraint_get_constant_val(c);
      valdim = isl_val_div(valdim,
          isl_val_neg(isl_constraint_get_coefficient_val(c, isl_dim_out, (int)dimi)));
      isl_constraint_free(c);
      break;
    }
    isl_constraint_free(c);
  }
  isl_constraint_list_free(cl);
  isl_basic_set_free(bs);
  isl_basic_set_list_free(bsl);
  return valdim;
}

static void min_max_scan(isl_set *scan_domain,
    intmax_t ***min_max) {
  *min_max = malloc(2*sizeof(**min_max));
  isl_set *lexmin = isl_set_lexmin(isl_set_copy(scan_domain));
  isl_set *lexmax = isl_set_lexmax(isl_set_copy(scan_domain));
  unsigned int ndims = isl_set_n_dim(scan_domain);
  (*min_max)[0] = malloc(ndims*sizeof(***min_max));
  (*min_max)[1] = malloc(ndims*sizeof(***min_max));
  for (unsigned int i = 0; i < ndims; ++i) {
    isl_val *min_val = get_val_dim(lexmin, i);
    isl_val *max_val = get_val_dim(lexmax, i);
    assert(isl_val_is_int(min_val));
    assert(isl_val_is_int(max_val));
    intmax_t min = (intmax_t) isl_val_get_num_si(min_val);
    intmax_t max = (intmax_t) isl_val_get_num_si(max_val);
    (*min_max)[0][i] = min;
    (*min_max)[1][i] = max;
    isl_val_free(min_val);
    isl_val_free(max_val);
  }
  isl_set_free(lexmin);
  isl_set_free(lexmax);
}

static void acr_specialize_id(size_t id,
    intmax_t min, intmax_t max, CloogUnionDomain *ud) {

  /*fprintf(stderr, "id%zu min %"PRIdMAX" max %"PRIdMAX"\n", id, min, max-1);*/

  isl_val *min_val = isl_val_int_from_si(isl_set_get_ctx((isl_set*)ud->domain->domain), min);
  isl_val *max_val = isl_val_int_from_si(isl_val_get_ctx(min_val), max-1);

  CloogNamedDomainList *statements = ud->domain;
  while (statements) {
    isl_set *domain = (isl_set*) statements->domain;
    /*isl_set_print_internal(domain, stderr, 0);*/
    if (isl_set_n_dim(domain) > id) {
      isl_local_space *ls = isl_local_space_from_space(isl_set_get_space(domain));
      isl_constraint *c1 = isl_constraint_alloc_inequality(isl_local_space_copy(ls));
      isl_constraint *c2 = isl_constraint_alloc_inequality(ls);
      c1 = isl_constraint_set_constant_val(c1, isl_val_neg(min_val));
      c1 = isl_constraint_set_coefficient_si(c1, isl_dim_out, (int)id, 1);
      c2 = isl_constraint_set_constant_val(c2, max_val);
      c2 = isl_constraint_set_coefficient_si(c2, isl_dim_out, (int)id, -1);
      domain = isl_set_add_constraint(domain, c1);
      domain = isl_set_add_constraint(domain, c2);
      statements->domain = (CloogDomain*) domain;
    }
    /*isl_set_print_internal(domain, stderr, 10);*/
    statements = statements->next;
  }
}

static void acr_fill_temporary_buffer_with_scan(FILE *bs,
    CloogOptions *options,
    CloogUnionDomain *scan,
    isl_set *context_scan,
    osl_scop_t *scan_scop) {
  options->scop = scan_scop;
  CloogProgram *scan_program =
    cloog_program_alloc((CloogDomain*)isl_set_copy(context_scan),
        scan, options);
  scan_program = cloog_program_generate(scan_program, options);
  cloog_program_pprint(bs, scan_program, options);
  cloog_program_free(scan_program);
}

static void acr_function_print_static_function(
    FILE *out,
    CloogOptions *options,
    CloogUnionDomain *ud,
    isl_set *context_ud,
    osl_scop_t *ud_scop,
    const char *scan_loop_nest,
    const size_t alternative_num,
    size_t tile_id,
    const char *function_parameters,
    const enum acr_static_data_reduction_function reduction_function) {

  fprintf(out, "void a%zu_%zu%s {\n",
      alternative_num, tile_id, function_parameters);

  options->scop = ud_scop;
  CloogProgram *ud_program =
    cloog_program_alloc((CloogDomain*)isl_set_copy(context_ud),
        ud, options);
  ud_program = cloog_program_generate(ud_program, options);
  cloog_program_pprint(out, ud_program, options);
  cloog_program_free(ud_program);

  if (reduction_function == acr_reduction_avg)
    fprintf(out, "size_t __acr_num_values = 0;\n");
  fprintf(out,
      "size_t __acr_tmp = 0;\n"
      "%s", scan_loop_nest);
  if (reduction_function == acr_reduction_avg)
    fprintf(out, "__acr_tmp = __acr_tmp / __acr_num_values;\n");
  fprintf(out, "*__acr_function_pointer = __acr_tmp_to_function(\n"
      "  %zu,\n"
      "  __acr_tmp,\n"
      "  __acr_function_array);\n"
      "}\n"
      , tile_id);
}

static void acr_apply_alternative_to_cloog_ud(CloogUnionDomain *ud,
    const struct runtime_alternative *al) {
  isl_val *parameter_value =
        isl_val_int_from_si(isl_set_get_ctx((isl_set*)ud->domain->domain),
            al->value.alt.parameter.parameter_value);
  isl_val *negone = isl_val_negone(isl_set_get_ctx((isl_set*)ud->domain->domain));
  CloogNamedDomainList *statement;
  switch (al->type) {
    case acr_runtime_alternative_parameter:
      statement = ud->domain;
      while (statement) {
        isl_set *domain = (isl_set*) statement->domain;
        isl_map *scatt = (isl_map*) statement->scattering;
        scatt = isl_map_remove_dims(scatt, isl_dim_param, 0, 1);
        isl_local_space *const lspace =
          isl_local_space_from_space(isl_set_get_space(domain));
        isl_constraint *parameter_constraint =
          isl_constraint_alloc_equality(lspace);
        parameter_constraint =
          isl_constraint_set_constant_val(parameter_constraint,
              isl_val_copy(parameter_value));
        parameter_constraint =
          isl_constraint_set_coefficient_val(parameter_constraint, isl_dim_param,
              0, isl_val_copy(negone));
        domain = isl_set_add_constraint(domain, parameter_constraint);
        domain = isl_set_project_out(domain, isl_dim_param, 0, 1);
        statement->domain = (CloogDomain*) domain;
        statement->scattering = (CloogScattering*) scatt;
        statement = statement->next;
      }
      break;
    case acr_runtime_alternative_function:
      fprintf(stderr, "[ACR] errot: Alternative function not yet supported\n");
      exit(EXIT_FAILURE);
      break;
  }
  isl_val_free(parameter_value);
  isl_val_free(negone);
}

static const char* acr_string_goto_first_for(char const* string) {
  size_t position = 0;
  while (string[position] != 'f' || string[position+1] != 'o' || string[position+2] != 'r') {
    position += 1;
  }
  return string + position;
}

static void acr_generate_tile_at_level(
    FILE *tile_library,
    FILE *temp_buffer,
    char **temp_buffer_string,
    CloogOptions *options,
    const size_t alternative_num,
    const size_t level,
    const intmax_t tiling_size,
    const size_t first_monitor_dimension,
    const size_t number_of_monitoring_dimensions,
    CloogUnionDomain *ud,
    CloogUnionDomain *scan,
    isl_set *context_ud,
    isl_set *context_scan,
    osl_scop_t *ud_scop,
    osl_scop_t *scan_scop,
    intmax_t *const*const min_max,
    size_t *const num_tiles,
    const enum acr_static_data_reduction_function reduction_function,
    struct runtime_alternative *alts,
    char const*const function_parameters) {

  intmax_t min = min_max[0][level];
  intmax_t max = min_max[1][level];
  for (intmax_t minval = min; minval <= max; minval += tiling_size) {
    CloogUnionDomain *ud_specialized = acr_copy_cloog_ud(ud);
    CloogUnionDomain *scan_specialized = acr_copy_cloog_ud(scan);
    acr_specialize_id(level, minval, minval+tiling_size, scan_specialized);
    acr_specialize_id(level+first_monitor_dimension, minval,
        minval+tiling_size, ud_specialized);
    if (level < number_of_monitoring_dimensions - 1) {
      if (!isl_set_is_empty((isl_set*)scan_specialized->domain->domain)) {
        acr_generate_tile_at_level(
            tile_library,
            temp_buffer,
            temp_buffer_string,
            options,
            alternative_num,
            level + 1,
            tiling_size,
            first_monitor_dimension,
            number_of_monitoring_dimensions,
            ud_specialized,
            scan_specialized,
            context_ud,
            context_scan,
            ud_scop,
            scan_scop,
            min_max,
            num_tiles,
            reduction_function,
            alts,
            function_parameters);
      }
      cloog_union_domain_free(scan_specialized);
      cloog_union_domain_free(ud_specialized);
    } else {
      if (isl_set_is_empty((isl_set*)scan_specialized->domain->domain)) {
        cloog_union_domain_free(scan_specialized);
        cloog_union_domain_free(ud_specialized);
      } else {
        fseek(temp_buffer, 0, SEEK_SET);
        acr_fill_temporary_buffer_with_scan(temp_buffer, options,
            scan_specialized, context_scan, scan_scop);
        fflush(temp_buffer);
        char const* scan_str = acr_string_goto_first_for(*temp_buffer_string);
        for (size_t i = 0; i < alternative_num-1; ++i) {
          CloogUnionDomain *ud_alt = acr_copy_cloog_ud(ud_specialized);
          acr_apply_alternative_to_cloog_ud(
              ud_alt, &alts[i]);
          acr_function_print_static_function(tile_library, options,
              ud_alt,
              context_ud,
              ud_scop,
              scan_str,
              i, *num_tiles, function_parameters, reduction_function);
        }
        acr_apply_alternative_to_cloog_ud(
            ud_specialized, &alts[alternative_num-1]);
        acr_function_print_static_function(tile_library, options,
            ud_specialized,
            context_ud,
            ud_scop,
            scan_str,
            alternative_num - 1, *num_tiles, function_parameters, reduction_function);
        /*cloog_union_domain_free(ud_specialized);*/
        *num_tiles += 1;
      }
    }
  }
}

void acr_code_generation_generate_tiling_library(
    struct acr_runtime_data_static *static_data,
    char **tiles_functions) {

  acr_runtime_apply_tiling(
      static_data->grid_size,
      static_data->first_monitor_dimension,
      static_data->num_monitor_dimensions,
      static_data->union_domain);

  size_t size_tiles_functions;
  FILE *tile_library = open_memstream(tiles_functions, &size_tiles_functions);

  acr_print_tiling_library_preamble(tile_library);

  CloogUnionDomain *scan_ud;
  osl_scop_p new_scop;
  CloogDomain *scan_context;
  acr_runtime_apply_reduction_function(
      static_data->context,
      static_data->union_domain,
      static_data->first_monitor_dimension,
      static_data->num_monitor_dimensions,
      &scan_ud, &scan_context, &new_scop,
      static_data->scan_corpse, static_data->iterators);

  CloogOptions *cloog_option = cloog_options_malloc(static_data->state);
  cloog_option->quiet = 1;
  cloog_option->openscop = 1;
  cloog_option->otl = 1;
  cloog_option->language = 0;

  isl_set *ud_context =
    isl_set_from_cloog_domain(static_data->context);
  ud_context = isl_set_remove_dims(ud_context, isl_dim_param, 0, 1);
  static_data->context = cloog_domain_from_isl_set(ud_context);

  intmax_t **min_max;
  min_max_scan((isl_set*)scan_ud->domain->domain, &min_max);

  static_data->num_fun_per_alt = 0;
  char *tbs;
  size_t tbs_size;
  FILE *tb = open_memstream(&tbs, &tbs_size);
  acr_generate_tile_at_level(
      tile_library,
      tb,
      &tbs,
      cloog_option,
      static_data->num_alternatives,
      0,
      (intmax_t) static_data->grid_size,
      static_data->first_monitor_dimension,
      static_data->num_monitor_dimensions,
      static_data->union_domain,
      scan_ud,
      (isl_set*) static_data->context,
      (isl_set*) scan_context,
      static_data->scop,
      new_scop,
      min_max,
      &static_data->num_fun_per_alt,
      static_data->reduction_function,
      static_data->alternatives,
      static_data->function_parameters);
  free(min_max[0]);
  free(min_max[1]);
  free(min_max);
  free(cloog_option);
  fclose(tb);
  free(tbs);

  acr_print_tiling_library_postamble(tile_library, static_data);

  fclose(tile_library);
  cloog_union_domain_free(scan_ud);
  osl_scop_free(new_scop);
  cloog_domain_free(scan_context);
}
