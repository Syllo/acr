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

#include "acr/parser_utils.h"
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
  union {
    int integer;
    float floating_point;
    unsigned uinteger;
  } constant_value;
  acr_option option;
  struct {
    char* param;
    int val;
  } alternative_parameter;
  struct {
    char* function_to_swap;
    char* replacement_function;
  } alternative_function;
  struct parameter_declaration* parameter_decl;
  struct parameter_declaration_list* parameter_decl_list;
}

%token  STRING_LITERAL FUNC_NAME SIZEOF
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
%token ACR_UNKNOWN
%token ACR_MIN ACR_MAX

%token <identifier> IDENTIFIER
%token <constant_value> I_CONSTANT F_CONSTANT

%type <constant_value> constant pointer
%type <option> acr_alternative_options acr_option
%type <option> acr_strategy_options acr_monitor_options acr_init_option
%type <alternative_parameter> acr_alternative_parameter_swap
%type <alternative_function> acr_alternative_function_swap
%type <parameter_decl> parameter_declaration
%type <parameter_decl_list> parameter_declaration_list

%destructor { acr_free_option(&$$); } <option>
%destructor { free(&$$); } IDENTIFIER
%destructor { free($$.param); } <alternative_parameter>
%destructor {
              free($$.function_to_swap); free($$.replacement_function);
            } <alternative_function>
%destructor { free_param_declarations($$); } <parameter_decl>
%destructor { free_param_decl_list($$); } <parameter_decl_list>

%start acr_start

%%

acr_start
  : IGNORE
    {
    }
  | PRAGMA_ACR acr_option
    {
      parsing_pragma_acr = false;
      free($2);
    }
  | acr_start IGNORE
    {
    }
  | acr_start PRAGMA_ACR acr_option
    {
      parsing_pragma_acr = false;
      free($3);
    }
  | error
    {
      fprintf(stderr, "Empty file\n");
    }
  ;

acr_option
  : ACR_ALTERNATIVE acr_alternative_options
    {
      acr_print_debug(stdout, "Rule accepted acr_option alternative");
      $$ = $2;
    }
  | ACR_DESTROY
    {
      acr_print_debug(stdout, "Rule accepted acr_option destroy");
      $$ = acr_new_destroy(last_pragma_start_line);
    }
  | ACR_DESTROY error
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_destroy]);
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
    }
  | ACR_GRID '(' I_CONSTANT ')'
    {
      acr_print_debug(stdout, "Rule accepted acr_option grid");
      if ($3.integer <= 0) {
        fprintf(stderr, "[ACR] Error: the grid size must be positive\n");
        error_print_last_pragma();
        $$ = NULL;
      } else {
        $$ = acr_new_grid($3.integer);
      }
    }
  | ACR_GRID error
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_grid]);
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
    }
  | ACR_INIT acr_init_option
    {
      acr_print_debug(stdout, "Rule accepted acr_option init");
      $$ = $2;
    }
  | ACR_MONITOR acr_monitor_options
    {
      acr_print_debug(stdout, "Rule accepted acr_option monitor");
      $$ = $2;
    }
  | ACR_STRATEGY acr_strategy_options
    {
      acr_print_debug(stdout, "Rule accepted acr_option strategy");
      $$ = $2;
    }
  | error
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_unknown]);
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
    }
  ;

acr_alternative_options
  : IDENTIFIER '(' IDENTIFIER ',' acr_alternative_parameter_swap ')'
    {
      if (strcmp($1,
          acr_pragma_alternative_names[acr_alternative_parameter].name) != 0) {
          fprintf(stderr, "%s",
          acr_pragma_alternative_names[acr_alternative_parameter].error_message);
          $$ = NULL;
          free($1);
          free($3);
          free($5.param);
          YYERROR;
      } else {
        $$ = acr_new_alternative_parameter($1, $5.param, $5.val);
        free($1);
        free($3);
        free($5.param);
      }
    }
  | IDENTIFIER '(' IDENTIFIER ',' acr_alternative_function_swap ')'
    {
      if (strcmp($1,
          acr_pragma_alternative_names[acr_alternative_function].name) != 0) {
          fprintf(stderr, "%s",
          acr_pragma_alternative_names[acr_alternative_function].error_message);
          $$ = NULL;
          free($1);
          free($3);
          free($5.function_to_swap);
          free($5.replacement_function);
          YYERROR;
      } else {
        $$ = acr_new_alternative_function($1, $5.function_to_swap,
            $5.replacement_function);
        free($1);
        free($3);
        free($5.function_to_swap);
        free($5.replacement_function);
      }
    }

  | error '(' IDENTIFIER ',' acr_alternative_parameter_swap ')'
    {
      $$ = NULL;
      free($3);
      free($5.param);
      fprintf(stderr,
        "[ACR] Hint: give a name to your alternative construct.\n");
      error_print_last_pragma();
      yyerrok;
    }
  | error '(' IDENTIFIER ',' acr_alternative_function_swap ')'
    {
      $$ = NULL;
      free($3);
      free($5.function_to_swap);
      free($5.replacement_function);
      fprintf(stderr,
        "[ACR] Hint: give a name to your alternative construct.\n");
      error_print_last_pragma();
      yyerrok;
    }
  | IDENTIFIER '(' error ',' acr_alternative_parameter_swap ')'
    {
      $$ = NULL;
      free($1);
      free($5.param);
      fprintf(stderr, "[ACR] Hint: did you mean to use \"parameter\"?\n");
      error_print_last_pragma();
      yyerrok;
    }
  | IDENTIFIER '(' error ',' acr_alternative_function_swap ')'
    {
      $$ = NULL;
      free($1);
      free($5.function_to_swap);
      free($5.replacement_function);
      fprintf(stderr, "[ACR] Hint: did you mean to use \"function\"?\n");
      error_print_last_pragma();
      yyerrok;
    }
  | error
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_alternative]);
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
    }
  ;

acr_alternative_parameter_swap
  : IDENTIFIER '=' I_CONSTANT
    {
      $$.param = $1;
      $$.val = $3.integer;
    }
  ;

acr_alternative_function_swap
  : IDENTIFIER '=' IDENTIFIER
    {
      $$.function_to_swap = $1;
      $$.replacement_function = $3;
    }
  ;

acr_init_option
  : '(' IDENTIFIER
    { /* test si void */
      if (strcmp($2, "void") != 0) {
        yyerror("ACR init fonction must return void\n");
        free($2);
        YYERROR;
      }
    }
    IDENTIFIER '(' parameter_declaration_list ')' ')'
    {
      if ($6) {

      } else {
        free($2);
        free($4);
      }
    }
  | error
    {
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_init]);
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
    }
  ;

parameter_declaration_list
  : parameter_declaration
    {
      $$ = add_declaration_to_list(NULL, $1);
    }
  | parameter_declaration_list ',' parameter_declaration
    {
      $$ = add_declaration_to_list($1, $3);
    }
  | error
    {
      $$ = NULL;
      fprintf(stderr, "[ACR] Hint: take a look at the init function"
      " declaration");
      error_print_last_pragma();
      yyerrok;
    }
  ;

parameter_declaration
  : parameter_declaration IDENTIFIER pointer
    {
      $$ = add_param_declaration($1, $2, $3.uinteger);
    }
  | parameter_declaration IDENTIFIER
    {
      $$ = add_param_declaration($1, $2, 0u);
    }
  | IDENTIFIER pointer
    {
      $$ = add_param_declaration(NULL, $1, $2.uinteger);
    }
  | IDENTIFIER
    {
      $$ = add_param_declaration(NULL, $1, 0u);
    }
  ;

pointer
  : '*'
    {
      $$.uinteger = 1;
    }
  | pointer '*'
    {
      $$.uinteger = $1.uinteger + 1;
    }
  ;

acr_monitor_options
  : '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ')'
    {
    }
  | '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ',' acr_monitor_filter ')'
    {
    }
  | error
    {
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_monitor]);
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
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
  | array_dimensions '[' error ']'
    {
      fprintf(stderr, "[ACR] Hint: declare the array dimensions with integers\n");
      error_print_last_pragma();
      yyerrok;
    }
  | '[' error ']'
    {
      fprintf(stderr, "[ACR] Hint: declare the array dimensions with integers\n");
      error_print_last_pragma();
      yyerrok;
    }
  ;

acr_monitor_filter
  : IDENTIFIER
    {
    }
  ;

acr_monitor_processing_function
  : IDENTIFIER
    {
      for(int i = acr_monitor_function_min; i < acr_monitor_function_unknown;
      ++i) {
        if (strcmp($1, acr_pragma_processing_functions[i]) == 0) {
        }
        else {
          fprintf(stderr, "[ACR] Error: ACR monitor does not handle %s\n", $1);
          YYERROR;
        }
      }
    }
  ;

acr_strategy_options
  : IDENTIFIER '(' constant ',' IDENTIFIER ')'
    {
      if (strcmp($1, acr_pragma_strategy_names[acr_strategy_direct].name) != 0) {
        fprintf(stderr, "%s",
          acr_pragma_strategy_names[acr_strategy_direct].error_message);
      }
    }
  | IDENTIFIER '(' constant ',' constant ',' IDENTIFIER ')'
    {
      if (strcmp($1, acr_pragma_strategy_names[acr_strategy_range].name) != 0) {
        fprintf(stderr, "%s",
          acr_pragma_strategy_names[acr_strategy_range].error_message);
      }
    }
  | error
    {
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_strategy]);
      scanner_clean_until_new_line();
      error_print_last_pragma();
      yyclearin;
      yyerrok;
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
  char row_buffer[100]; // Buffered read

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

  unsigned int underline_size = last_token_size > 0 ? last_token_size - 1 : 0;
  for (unsigned int j = 0; j < underline_size; ++j) {
    fprintf(stderr, "%c", '~');
  }
  fprintf(stderr, "\n");
  fclose(file);
}
