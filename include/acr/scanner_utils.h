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

/**
 *
 * \file scanner_utils.h
 * \brief Lex useful data
 *
 * \defgroup build_lexyacc
 *
 * @{
 *
 */

#ifndef __ACR_SCANNER_UTILS_H
#define __ACR_SCANNER_UTILS_H

#include "acr/pragma_struct.h"
#include "acr_parser.h"

struct acr_pragma_options_utils {
  char* name;
  int token_id;
};

static const struct acr_pragma_options_utils acr_pragma_options_name_list[] =
  {
    [acr_type_alternative]      = {"alternative",      ACR_ALTERNATIVE},
    [acr_type_deferred_destroy] = {"deferred_destroy", ACR_DEF_DESTROY},
    [acr_type_destroy]          = {"destroy",          ACR_DESTROY},
    [acr_type_grid]             = {"grid",             ACR_GRID},
    [acr_type_init]             = {"init",             ACR_INIT},
    [acr_type_monitor]          = {"monitor",          ACR_MONITOR},
    [acr_type_strategy]         = {"strategy",         ACR_STRATEGY},
    [acr_type_unknown]          = {"unknown",          ACR_UNKNOWN},
  };

#endif // __ACR_SCANNER_UTILS_H

/**
 *
 * @}
 *
 */
