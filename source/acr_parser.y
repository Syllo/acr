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
#include <stdio.h>
#include <string.h>


#include "acr/pragma_struct.h"
#include "acr/utils.h"

const char* current_file = "test.c"; //placeholder

extern void scanner_clean_until_new_line(void);
void yyerror(const char *);  /* prints grammar violation message */
int yylex(void);
static void error_print_last_pragma(void);

bool parsing_pragma_acr;
extern size_t position_in_file;
extern size_t position_of_last_starting_row;
extern size_t position_scanning_row;
extern size_t position_scanning_column;
extern size_t position_start_current_token;
extern size_t last_token_size;
extern size_t last_pragma_start_line;

%}

%union {
  char* identifier;
}

%token  I_CONSTANT F_CONSTANT STRING_LITERAL FUNC_NAME SIZEOF
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
%token ACR_PARAMETER ACR_FUNCTION ACR_UNKNOWN
%token ACR_MIN ACR_MAX ACR_DIRECT ACR_RANGE

%token <identifier> IDENTIFIER

%start acr_start

%%

acr_start
  : IGNORE
    {
    }
  | PRAGMA_ACR acr_option
    {
      parsing_pragma_acr = false;
    }
  | acr_start IGNORE
    {
    }
  | acr_start PRAGMA_ACR acr_option
    {
      parsing_pragma_acr = false;
    }
  | {  }
  ;

acr_option
  : ACR_ALTERNATIVE acr_alternative_options
    {
      acr_print_debug(stdout, "Rule accepted acr_option alternative");
    }
  | ACR_DESTROY
    {
      acr_print_debug(stdout, "Rule accepted acr_option destroy");
    }
  | ACR_GRID '(' I_CONSTANT ')'
    {
      acr_print_debug(stdout, "Rule accepted acr_option grid");
    }
  | ACR_GRID '(' error ')'
    {
      fprintf(stderr, "[ACR] Hint: define the grid using an integer\n");
      error_print_last_pragma();
    }
  | ACR_INIT acr_init_option
    {
      acr_print_debug(stdout, "Rule accepted acr_option init");
    }
  | ACR_MONITOR acr_monitor_options
    {
      acr_print_debug(stdout, "Rule accepted acr_option monitor");
    }
  | ACR_STRATEGY acr_strategy_options
    {
      acr_print_debug(stdout, "Rule accepted acr_option strategy");
    }
  | error
    {
      fprintf(stderr, "[ACR] Warning: unrecognized pragma\n");
      error_print_last_pragma();
      scanner_clean_until_new_line();
      yyclearin;
      yyerrok;
    }
  ;

acr_alternative_options
  : IDENTIFIER '(' ACR_PARAMETER ',' acr_alternative_parameter_swap ')'
  | IDENTIFIER '(' ACR_FUNCTION ',' acr_alternative_function_swap ')'
  | error '(' ACR_PARAMETER ',' acr_alternative_parameter_swap ')'
    {
      fprintf(stderr,
        "[ACR] Hint: give a name to your alternative construct.\n");
      error_print_last_pragma();
      yyerrok;
    }
  | error '(' ACR_FUNCTION ',' acr_alternative_function_swap ')'
    {
      fprintf(stderr,
        "[ACR] Hint: give a name to your alternative construct.\n");
      error_print_last_pragma();
      yyerrok;
    }
  | error
    {
      fprintf(stderr, "[ACR] Hint: did you forget the name or a parameter"
      " in the alternative construct?\n");
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
    }
  ;

acr_alternative_parameter_swap
  : IDENTIFIER '=' I_CONSTANT
    {
    }
  | IDENTIFIER '=' error
    {
      fprintf(stderr, "[ACR] Hint: parameter swap needs an integer");
      yyerrok;
    }
  ;

acr_alternative_function_swap
  : IDENTIFIER '=' IDENTIFIER
    {
    }
  ;

acr_init_option
  : IDENTIFIER
    { /* test si void */
      if (strcmp($1, "void") != 0) {
        yyerror("ACR init fonction must return void\n");
        YYERROR;
      }
    }
    IDENTIFIER '(' parameter_declaration_list ')'
    {
    }
  ;

parameter_declaration_list
  : parameter_declaration
    {
    }
  | parameter_declaration_list ',' parameter_declaration
    {
    }
  ;

parameter_declaration
  : parameter_declaration IDENTIFIER pointer
    {
    }
  | parameter_declaration IDENTIFIER
    {
    }
  | IDENTIFIER pointer
    {
    }
  | IDENTIFIER
    {
    }
  ;

pointer
  : '*'
    {
    }
  | pointer '*'
    {
    }
  ;

acr_monitor_options
  : '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ')'
    {
    }
  | '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ',' acr_monitor_filter ')'
    {
    }
  ;
acr_monitor_data_monitored
  : parameter_declaration array_dimensions
    {
    }
  ;

array_dimensions
  : array_dimensions '[' I_CONSTANT ']'
    {
    }
  | '[' I_CONSTANT ']'
    {
    }
  ;

acr_monitor_filter
  : IDENTIFIER
    {
    }
  ;

acr_monitor_processing_function
  : ACR_MIN
    {
    }
  | ACR_MAX
    {
    }
  ;

acr_strategy_options
  : ACR_DIRECT '(' constant ',' IDENTIFIER ')'
    {
    }
  | ACR_RANGE '(' constant ',' constant ',' IDENTIFIER ')'
    {
    }
  ;

constant
  : I_CONSTANT
    {
    }
  | F_CONSTANT
    {
    }
  ;


%%

#include <stdio.h>

static void error_print_last_pragma(void) {
  FILE* file;
  file = fopen(current_file, "r");
  fseek(file, last_pragma_start_line, SEEK_SET);
  char c;
  char previous;

  fprintf(stderr, "[ACR] Ignoring following pragma:\n*\n");

  fscanf(file, "%c", &c);
  fprintf(stderr, "* %c", c);
  while(previous = c, fscanf(file, "%c", &c), c != EOF) {
    fprintf(stderr, "%c", c);
    if (previous != '\\' && c == '\n') {
      break;
    } else {
      if (c == '\n')
        fprintf(stderr, "* ");
    }
  }
  fflush(stderr);
  fclose(file);
}

void yyerror(const char *s)
{
  FILE* file;
  char row_buffer[100]; // most rows should be less than that

  fflush(stdout);
  fprintf(stderr, "[ACR] Error: %s\n", s);
  fprintf(stderr, "[ACR] Error occured at line %zu column %zu\n",
      position_scanning_row + 1,
      position_scanning_column + 1 - last_token_size);

  file = fopen(current_file, "r");
  fseek(file, position_of_last_starting_row, SEEK_SET);

  unsigned int i = 101;
  do {
    if (i == 101) {
      fscanf(file, "%100c", row_buffer);
      i = 1;
    }
    else
      ++i;

    fprintf(stderr, "%c", row_buffer[i - 1]);
  } while (row_buffer[i] != '\0' && row_buffer[i] != '\n');

  fprintf(stderr, "\n");

  for (unsigned int j = 0; j < position_start_current_token; ++j) {
    fprintf(stderr, "%c", ' ');
  }

  fprintf(stderr, "%c", '^');

  for (unsigned int j = 0; j < last_token_size - 1; ++j) {
    fprintf(stderr, "%c", '~');
  }
  fprintf(stderr, "\n");
  fclose(file);
}
