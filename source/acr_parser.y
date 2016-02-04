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

%{

#include <stdbool.h>
#include <string.h>

#include "acr/pragma_struct.h"

void yyerror(const char *);  /* prints grammar violation message */
int yylex(void);

bool parsing_pragma_acr;

%}

%token  IDENTIFIER I_CONSTANT F_CONSTANT STRING_LITERAL FUNC_NAME SIZEOF
%token  PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token  AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token  SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token  XOR_ASSIGN OR_ASSIGN
%token  TYPEDEF_NAME ENUMERATION_CONSTANT

%token  TYPEDEF EXTERN STATIC AUTO REGISTER INLINE
%token  CONST RESTRICT VOLATILE
%token  BOOL CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE VOID
%token  COMPLEX IMAGINARY
%token  STRUCT UNION ENUM ELLIPSIS

%token  CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token  ALIGNAS ALIGNOF ATOMIC GENERIC NORETURN STATIC_ASSERT THREAD_LOCAL

%token IGNORE
%token PRAGMA_ACR
%token ACR_INIT ACR_DESTROY ACR_STRATEGY ACR_ALTERNATIVE ACR_MONITOR ACR_GRID
%token ACR_PARAMETER ACR_FUNCTION
%token ACR_MIN ACR_MAX ACR_DIRECT ACR_RANGE

%start acr_start

%%

acr_start
  : IGNORE {  }
  | PRAGMA_ACR acr_option
    {
      parsing_pragma_acr = false;
    }
  | acr_start IGNORE {  }
  | acr_start PRAGMA_ACR acr_option
    {
      parsing_pragma_acr = false;
    }
  ;

acr_option
  : ACR_ALTERNATIVE acr_alternative_options
  | ACR_DESTROY
  | ACR_GRID '(' I_CONSTANT ')'
  | ACR_INIT acr_init_option
  | ACR_MONITOR acr_monitor_options
  | ACR_STRATEGY acr_strategy_options
  ;

acr_alternative_options
  : IDENTIFIER '(' ACR_PARAMETER ',' acr_alternative_parameter_swap ')'
  | IDENTIFIER '(' ACR_FUNCTION ',' acr_alternative_function_swap ')'
  ;

acr_alternative_parameter_swap
  : IDENTIFIER '=' I_CONSTANT
  ;

acr_alternative_function_swap
  : IDENTIFIER '=' IDENTIFIER
  ;

acr_init_option
  : IDENTIFIER
    { /* test si void */
      if (strcmp(yylval, "void") != 0) {
        yyerror("Error: acr init fonction must return void\n");
      }
    }
    IDENTIFIER '(' parameter_declaration_list ')'
  ;

parameter_declaration_list
  : parameter_declaration
  | parameter_declaration_list ',' parameter_declaration
  ;

parameter_declaration
  : parameter_declaration IDENTIFIER pointer
  | parameter_declaration IDENTIFIER
  | IDENTIFIER pointer
  | IDENTIFIER
  ;

pointer
  : '*'
  | pointer '*'
  ;

acr_monitor_options
  : '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ')'
  | '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ',' acr_monitor_filter ')'
  ;
acr_monitor_data_monitored
  : parameter_declaration array_dimensions
  ;

array_dimensions
  : array_dimensions '[' I_CONSTANT ']'
  | '[' I_CONSTANT ']'
  ;

acr_monitor_filter
  : IDENTIFIER
  ;

acr_monitor_processing_function
  : ACR_MIN
  | ACR_MAX
  ;

acr_strategy_options
  : ACR_DIRECT '(' constant ',' IDENTIFIER ')'
  | ACR_RANGE '(' constant ',' constant ',' IDENTIFIER ')'
  ;

constant
  : I_CONSTANT
  | F_CONSTANT
  ;


%%

#include <stdio.h>

void yyerror(const char *s)
{
  fflush(stdout);
  fprintf(stderr, "*** %s\n", s);
}
