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
#include "acr/print.h" // for debug

static const char* acr_pragma_options_error_messages[] =
  {
    [acr_type_alternative] = "[ACR] Hint: take a look at the"
                             " alternative construct\n",
    [acr_type_destroy]     = "[ACR] Hint: take a look at the"
                             " destroy construct\n",
    [acr_type_grid]        = "[ACR] Hint: take a look at the"
                             " grid construct\n",
    [acr_type_init]        = "[ACR] Hint: take a look at the"
                             " init construct\n",
    [acr_type_monitor]     = "[ACR] Hint: take a look at the"
                             " monitor construct\n",
    [acr_type_strategy]    = "[ACR] Hint: take a look at the"
                             " strategy construct\n",
    [acr_type_unknown]     = "[ACR] Warning: unrecognized or"
                             " malformed pragma\n",
  };

static const char* acr_pragma_processing_functions[] =
  {
    [acr_monitor_function_min]  = "min",
    [acr_monitor_function_max]  = "max",
  };

void yylex_destroy(void);
void yyerror(const char *);  /* prints grammar violation message */
int yylex(void);
static void error_print_last_pragma(void);
static void handle_carriage_return(void);

bool parsing_pragma_acr;
extern size_t position_in_file;
extern size_t position_of_last_starting_row;
extern size_t position_scanning_row;
extern size_t position_scanning_column;
extern size_t position_start_current_token;
extern size_t last_token_size;
extern size_t last_pragma_start_line;
extern FILE* yyin;

struct parser_option_list* option_list;

%}

%union {
  char* identifier;
  struct {
    enum {integer_value, floating_point_value} type;
    union {
      float floating_point;
      struct {
        long int integer;
        unsigned long uinteger;
      } integer_val;
    } value;
  } constant_value;
  acr_option option;
  struct {
    char* param;
    long int val;
  } alternative_parameter;
  struct {
    char* function_to_swap;
    char* replacement_function;
  } alternative_function;
  struct parameter_declaration* parameter_decl;
  struct parameter_declaration_list* parameter_decl_list;
  struct acr_array_declaration array_declaration;
  struct array_dimensions* array_dimensions;
  int monitor_processing_function;
  bool minus;
}

%token  CARRIAGE_RETURN
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

%type <identifier> acr_monitor_filter
%type <constant_value> constant pointer
%type <minus> minus
%type <option> acr_alternative_options acr_option
%type <option> acr_strategy_options acr_monitor_options acr_init_option
%type <alternative_parameter> acr_alternative_parameter_swap
%type <alternative_function> acr_alternative_function_swap
%type <parameter_decl> parameter_declaration
%type <parameter_decl_list> parameter_declaration_list
%type <array_declaration> acr_monitor_data_monitored
%type <array_dimensions> array_dimensions
%type <monitor_processing_function> acr_monitor_processing_function

%destructor { acr_free_option($$); $$ = NULL;} <option>
%destructor { free($$); $$ = NULL;} <identifier>
%destructor { free($$.param); $$.param = NULL;} <alternative_parameter>
%destructor {
              free($$.function_to_swap); $$.function_to_swap = NULL;
              free($$.replacement_function); $$.replacement_function = NULL;
            } <alternative_function>
%destructor { free_param_declarations($$); $$ = NULL;} <parameter_decl>
%destructor { free_param_decl_list($$); $$ = NULL;} <parameter_decl_list>
%destructor { acr_free_acr_array_declaration(&$$); } <array_declaration>
%destructor { free_dimensions($$); $$ = NULL;} <array_dimensions>

%start acr_start

%%

acr_start
  : IGNORE
    {
    }
  | PRAGMA_ACR acr_option
    {
      handle_carriage_return();
      parsing_pragma_acr = false;
      if ($2) {
        option_list = parser_option_list_add($2, option_list);
      }
    }
  | acr_start IGNORE
    {
    }
  | acr_start PRAGMA_ACR acr_option
    {
      handle_carriage_return();
      parsing_pragma_acr = false;
      if ($3) {
        option_list = parser_option_list_add($3, option_list);
      }
    }
  | error
    {
      fprintf(stderr, "Empty file\n");
    }
  ;

acr_option
  : ACR_ALTERNATIVE acr_alternative_options CARRIAGE_RETURN
    {
      acr_print_debug(stdout, "Rule accepted acr_option alternative");
      $$ = $2;
    }
  | ACR_ALTERNATIVE error CARRIAGE_RETURN
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_alternative]);
      error_print_last_pragma();
      yyerrok;
    }
  | ACR_DESTROY CARRIAGE_RETURN
    {
      acr_print_debug(stdout, "Rule accepted acr_option destroy");
      $$ = acr_new_destroy(last_pragma_start_line);
    }
  | ACR_DESTROY error CARRIAGE_RETURN
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_destroy]);
      error_print_last_pragma();
      yyerrok;
    }
  | ACR_GRID '(' I_CONSTANT ')' CARRIAGE_RETURN
    {
      acr_print_debug(stdout, "Rule accepted acr_option grid");
      if ($3.value.integer_val.integer <= 0) {
        fprintf(stderr, "[ACR] Error: the grid size must be positive\n");
        error_print_last_pragma();
        $$ = NULL;
      } else {
        $$ = acr_new_grid($3.value.integer_val.uinteger);
      }
    }
  | ACR_GRID error CARRIAGE_RETURN
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_grid]);
      error_print_last_pragma();
      yyerrok;
    }
  | ACR_INIT acr_init_option CARRIAGE_RETURN
    {
      acr_print_debug(stdout, "Rule accepted acr_option init");
      $$ = $2;
    }
  | ACR_INIT error CARRIAGE_RETURN
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_init]);
      error_print_last_pragma();
      yyerrok;
    }
  | ACR_MONITOR acr_monitor_options CARRIAGE_RETURN
    {
      acr_print_debug(stdout, "Rule accepted acr_option monitor");
      $$ = $2;
    }
  | ACR_MONITOR error CARRIAGE_RETURN
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_monitor]);
      error_print_last_pragma();
      yyerrok;
    }
  | ACR_STRATEGY acr_strategy_options CARRIAGE_RETURN
    {
      acr_print_debug(stdout, "Rule accepted acr_option strategy");
      $$ = $2;
    }
  | ACR_STRATEGY error CARRIAGE_RETURN
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_strategy]);
      error_print_last_pragma();
      yyerrok;
    }
  | error CARRIAGE_RETURN
    {
      $$ = NULL;
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_unknown]);
      error_print_last_pragma();
      yyerrok;
    }
  ;

acr_alternative_options
  : IDENTIFIER '(' IDENTIFIER ',' acr_alternative_parameter_swap ')'
    {
      if (strcmp($3,
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
      if (strcmp($3,
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
  | IDENTIFIER '(' error  ')'
    {
      $$ = NULL;
      free($1);
      fprintf(stderr, "%s",
        acr_pragma_options_error_messages[acr_type_alternative]);
      error_print_last_pragma();
      yyerrok;
    }
  ;

acr_alternative_parameter_swap
  : IDENTIFIER '=' minus I_CONSTANT
    {
      $$.param = $1;
      $$.val = $4.value.integer_val.integer;
      if ($3)
        $$.val *= -1l;
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
        yyerror("ACR init fonction must return void");
        YYERROR;
      }
    }
    IDENTIFIER '(' parameter_declaration_list ')' ')'
    {
      if ($6) {
        acr_parameter_declaration* parameter_list;
        unsigned long int num_parameters =
            translate_and_free_param_declaration_list($6, &parameter_list);
        $$ = acr_new_init($4, last_pragma_start_line, num_parameters,
                          parameter_list);
      }
      free($2);
      free($4);
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
  ;

parameter_declaration
  : parameter_declaration IDENTIFIER pointer
    {
      $$ = add_param_declaration($1, $2, $3.value.integer_val.uinteger);
    }
  | parameter_declaration IDENTIFIER
    {
      $$ = add_param_declaration($1, $2, 0u);
    }
  | IDENTIFIER pointer
    {
      $$ = add_param_declaration(NULL, $1, $2.value.integer_val.uinteger);
    }
  | IDENTIFIER
    {
      $$ = add_param_declaration(NULL, $1, 0u);
    }
  ;

pointer
  : '*'
    {
      $$.value.integer_val.uinteger = 1;
    }
  | pointer '*'
    {
      $$.value.integer_val.uinteger = $1.value.integer_val.uinteger + 1ul;
    }
  ;

acr_monitor_options
  : '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ')'
    {
      $$ = acr_new_monitor(&$2, $4, NULL);
    }
  | '(' acr_monitor_data_monitored ',' acr_monitor_processing_function ',' acr_monitor_filter ')'
    {
      $$ = acr_new_monitor(&$2, $4, $6);
      free($6);
    }
  ;
acr_monitor_data_monitored
  : parameter_declaration array_dimensions
    {
      $$.num_specifiers =
      get_name_and_specifiers_and_free_parameter_declaration($1,
      &($$.array_name),
          &$$.parameter_specifiers_list);
      $$.num_dimensions = get_size_and_dimensions_and_free($2,
          &$$.array_dimensions_list);
    }
  ;

array_dimensions
  : array_dimensions '[' I_CONSTANT ']'
    {
      $$ = add_dimension_uinteger($1, $3.value.integer_val.uinteger);
    }
  | '[' I_CONSTANT ']'
    {
      $$ = add_dimension_uinteger(NULL, $2.value.integer_val.uinteger);
    }
  | '[' IDENTIFIER ']'
    {
      $$ = add_dimension_name(NULL, $2);
      free($2);
    }
  | array_dimensions '[' IDENTIFIER ']'
    {
      $$ = add_dimension_name($1, $3);
      free($3);
    }
  | array_dimensions '[' error ']'
    {
      $$ = NULL;
      free_dimensions($1);
      fprintf(stderr, "[ACR] Hint: declare the array dimensions with positive"
      " integers or parameter name\n");
      error_print_last_pragma();
      yyerrok;
    }
  | '[' error ']'
    {
      $$ = NULL;
      fprintf(stderr, "[ACR] Hint: declare the array dimensions with integers"
      " or parameter name\n");
      error_print_last_pragma();
      yyerrok;
    }
  ;

acr_monitor_filter
  : IDENTIFIER
    {
      $$ = $1;
    }
  ;

acr_monitor_processing_function
  : IDENTIFIER
    {
      bool found = false;
      for(int i = acr_monitor_function_min; i < acr_monitor_function_unknown;
      ++i) {
        if (strcmp($1, acr_pragma_processing_functions[i]) == 0) {
          $$ = i;
          found = true;
          break;
        }
      }
      if (!found) {
        fprintf(stderr, "[ACR] Error: ACR monitor does not handle the %s"
        " function\n", $1);
        free($1);
        YYERROR;
      }
      free($1);
    }
  ;

acr_strategy_options
  : IDENTIFIER '(' constant ',' IDENTIFIER ')'
    {
      if (strcmp($1, acr_pragma_strategy_names[acr_strategy_direct].name) != 0) {
        fprintf(stderr, "%s",
          acr_pragma_strategy_names[acr_strategy_direct].error_message);
          free($1);
          free($5);
          YYERROR;
      }
      if ($3.type == integer_value) {
        $$ = acr_new_strategy_direct_int($5, $3.value.integer_val.integer);
      } else {
        $$ = acr_new_strategy_direct_float($5, $3.value.floating_point);
      }
      free($1);
      free($5);
    }
  | IDENTIFIER '(' constant ',' constant ',' IDENTIFIER ')'
    {
      if (strcmp($1, acr_pragma_strategy_names[acr_strategy_range].name) != 0) {
        fprintf(stderr, "%s",
          acr_pragma_strategy_names[acr_strategy_range].error_message);
          free($1);
          free($7);
          YYERROR;
      }
      const char range_error[] = "[ACR] Error, lower bound is greater than"
          " the upper bound\n";

      if($3.type == floating_point_value) {
        if ($5.type == floating_point_value) {
          if ($3.value.floating_point > $5.value.floating_point) {
            fprintf(stderr, "%s", range_error);
            error_print_last_pragma();
            free($1);
            free($7);
            YYERROR;
          }
        } else {
          if ($3.value.floating_point >
              ((float) $5.value.integer_val.integer)) {
            fprintf(stderr, "%s", range_error);
            error_print_last_pragma();
            free($1);
            free($7);
            YYERROR;
          }
        }
      } else {
        if ($5.type == floating_point_value) {
          if (((float)$3.value.integer_val.integer) >
              $5.value.floating_point) {
            fprintf(stderr, "%s", range_error);
            error_print_last_pragma();
            free($1);
            free($7);
            YYERROR;
          }
        } else {
          if ($3.value.integer_val.integer > $5.value.integer_val.integer) {
            fprintf(stderr, "%s", range_error);
            error_print_last_pragma();
            free($1);
            free($7);
            YYERROR;
          }
        }
      }
      if ($3.type == floating_point_value || $5.type == floating_point_value) {
        float bounds[2];
        if ($3.type == integer_value)
          bounds[0] = (float) $3.value.integer_val.integer;
        else
          bounds[0] = $3.value.floating_point;
        if ($5.type == integer_value)
          bounds[1] = (float) $5.value.integer_val.integer;
        else
          bounds[1] = $5.value.floating_point;
        $$ = acr_new_strategy_range_float($7, bounds);
      } else {  // integer
        long int bounds[2];
        bounds[0] = $3.value.integer_val.integer;
        bounds[1] = $5.value.integer_val.integer;
        $$ = acr_new_strategy_range_int($7, bounds);
      }
      free($1);
      free($7);
    }
  ;

constant
  : minus I_CONSTANT
    {
      if ($1) {
        $2.value.integer_val.integer *= -1l;
      }
      $$ = $2;
    }
  | minus F_CONSTANT
    {
      if ($1) {
        $2.value.floating_point *= -1.f;
      }
      $$ = $2;
    }
  ;

minus
  : '-'
    {
      $$ = true;
    }
  | minus '-'
    {
      $$ ^= true;
    }
  | %empty
    {
      $$ = false;
    }
  ;


%%

static void error_print_last_pragma(void) {
  long current_position = ftell(yyin);
  fseek(yyin, last_pragma_start_line, SEEK_SET);
  char c;
  char previous;

  fprintf(stderr, "[ACR] Ignoring following pragma:\n*\n");

  fscanf(yyin, "%c", &c);
  fprintf(stderr, "* %c", c);
  while(previous = c, fscanf(yyin, "%c", &c), c != EOF) {
    fprintf(stderr, "%c", c);
    if (previous != '\\' && c == '\n') {
      break;
    } else {
      if (c == '\n')
        fprintf(stderr, "* ");
    }
  }
  fprintf(stderr, "*\n\n");
  fflush(stderr);
  fseek(yyin, current_position, SEEK_SET);
}

static void handle_carriage_return(void) {
  ++position_in_file;
  position_of_last_starting_row = position_in_file;
  ++position_scanning_row;
  position_scanning_column = 0;
}

void yyerror(const char *s)
{
  char row_buffer[100]; // Buffered read

  fflush(stdout);
  fprintf(stderr, "[ACR] Error: %s\n", s);
  fprintf(stderr, "[ACR] Error occured at line %zu column %zu\n",
      position_scanning_row + 1,
      position_scanning_column + 1 - last_token_size);

  long current_position = ftell(yyin);
  fseek(yyin, position_of_last_starting_row, SEEK_SET);

  if(position_in_file == 0)
    return;

  size_t i = 101;
  do {
    if (i == 101) {
      fscanf(yyin, "%100c", row_buffer);
      i = 1;
    }
    else
      ++i;

    fprintf(stderr, "%c", row_buffer[i - 1]);
  } while (row_buffer[i] != '\0' && row_buffer[i] != '\n');

  fprintf(stderr, "\n");

  for (size_t j = 0; j < position_start_current_token; ++j) {
    fprintf(stderr, "%c", ' ');
  }

  fprintf(stderr, "%c", '^');

  size_t underline_size = last_token_size > 0 ? last_token_size - 1 : 0;
  for (size_t j = 0; j < underline_size; ++j) {
    fprintf(stderr, "%c", '~');
  }
  fprintf(stderr, "\n");
  fseek(yyin, current_position, SEEK_SET);
}

int start_acr_parsing(FILE* file, acr_compute_node* node_to_init) {
  if (!node_to_init) {
    fprintf(stderr, "We need a node to init\n");
    return 1;
  }
  position_in_file = 0;
  position_of_last_starting_row = 0;
  position_scanning_row = 0;
  position_scanning_column = 0;
  position_start_current_token = 0;
  last_token_size = 0;
  yyin = file;
  option_list = NULL;
  int yyparseval = yyparse();
  if (yyparseval != 0) {
    fprintf(stderr, "Parser error, abort\n");
    parser_free_option_list(option_list);
  } else {
    acr_option_list new_option_list;
    unsigned long list_size = parser_translate_option_list_and_free(option_list,
        &new_option_list);
    if (new_option_list) {
      *node_to_init = acr_new_compute_node(list_size, new_option_list);
    } else {
      *node_to_init = NULL;
    }
  }
  yylex_destroy();
  return yyparseval;
}
