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

#ifndef __ACR_CLOOG_RUNTIME_H
#define __ACR_CLOOG_RUNTIME_H

#include <acr/runtime_alternatives.h>
#include <cloog/cloog.h>
#include <isl/set.h>

char* acr_cloog_generate_alternative_code_from_input(
    CloogState *state,
    CloogInput *cloog_input,
    unsigned long num_alternatives,
    struct runtime_alternative *alternatives,
    isl_set **sets);

#endif // __ACR_CLOOG_RUNTIME_H
