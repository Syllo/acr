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

#include "acr/parser_utils.h"

struct parameter_declaration* add_param_declaration(
    struct parameter_declaration* current, char* name,
    unsigned long int pointer_depth) {
  struct parameter_declaration* dec = malloc(sizeof(*dec));
  dec->name = name;
  dec->pointer_depth = pointer_depth;
  dec->next = current;
  if (current)
    current->previous = dec;
  dec->previous = NULL;
  return dec;
}

struct parameter_declaration_list* add_declaration_to_list(
    struct parameter_declaration_list* current,
    struct parameter_declaration* declaration) {
  struct parameter_declaration_list* dec = malloc(sizeof(*dec));
  dec->declaration = declaration;
  dec->next = current;
  if (current)
    current->previous = dec;
  dec->previous = NULL;
  return dec;
}

unsigned long int translate_and_free_param_declaration_list(
    struct parameter_declaration_list* list,
    acr_parameter_declaration** list_to_initialize) {
  if (!list) {
    *list_to_initialize = NULL;
    return 0;
  }

  unsigned long int size_list = 1;
  struct parameter_declaration_list* list_iterator = list;
  while (list_iterator->next) {
    list_iterator = list_iterator->next;
    ++size_list;
  }

  *list_to_initialize =
    acr_new_parameter_declaration_list(size_list);

  for (unsigned long int i = 0; i < size_list; ++i) {
    char* parameter_name;
    acr_parameter_specifier* specifier_list;
    unsigned long int num_specifiers =
      get_name_and_specifiers_and_free_parameter_declaration(
        list_iterator->declaration,
        &parameter_name,
        &specifier_list);
    acr_set_parameter_declaration(parameter_name, num_specifiers,
        specifier_list, &(*list_to_initialize)[i]);
    if (list_iterator->previous) {
      list_iterator = list_iterator->previous;
      free(list_iterator->next);
    }
  }
  free(list_iterator);
  return size_list;
}

unsigned long int get_name_and_specifiers_and_free_parameter_declaration(
    struct parameter_declaration* declaration,
    char** parameter_name,
    acr_parameter_specifier** specifier_list) {
  if (!declaration || !declaration->next) {
    if (declaration) {
      free(declaration->name);
      free(declaration);
    }
    *specifier_list = NULL;
    *parameter_name = NULL;
    return 0;
  }

  unsigned long int num_specifiers = 1;
  struct parameter_declaration* specifiers = declaration->next;
  while (specifiers->next) {
    specifiers = specifiers->next;
    ++num_specifiers;
  }

  *specifier_list = acr_new_parameter_specifier_list(num_specifiers);

  for (unsigned long int i = 0; i < num_specifiers; ++i) {
    acr_set_parameter_specifier(specifiers->name, specifiers->pointer_depth,
        &(*specifier_list)[i]);
    free(specifiers->name);
    specifiers = specifiers->previous;
    free(specifiers->next);
  }

  *parameter_name = specifiers->name;
  free(specifiers);

  return num_specifiers;
}

struct array_dimensions* add_dimension(struct array_dimensions* previous,
    unsigned long int dimension_size) {
  struct array_dimensions* new_dim = malloc(sizeof(*new_dim));
  new_dim->dimension = dimension_size;
  new_dim->next = previous;
  if (previous)
    previous->previous = new_dim;
  new_dim->previous = NULL;
  return new_dim;
}

unsigned long int get_size_and_dimensions_and_free(
    struct array_dimensions* dimension,
    unsigned long int** dimension_list) {
  unsigned long int num_dimensions = 0;
  if (dimension) {
    num_dimensions = 1;
    while (dimension->next) {
      dimension = dimension->next;
      ++num_dimensions;
    }

    *dimension_list = malloc(num_dimensions * sizeof(**dimension_list));

    for (unsigned long int i = 0; i < num_dimensions; ++i) {
      (*dimension_list)[i] = dimension->dimension;
      if (dimension->previous) {
        dimension = dimension->previous;
        free(dimension->next);
      }
    }
    free(dimension);
  }
  return num_dimensions;
}
