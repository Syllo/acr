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

O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\n\v\f]

%{

#include "acr/scanner_utils.h"

#include <stdbool.h>
#include <stdio.h>

#include "acr/utils.h"

extern char* current_file; // placeholder

extern void yyerror(const char *);  /* prints grammar violation message */

extern bool parsing_pragma_acr;

static int acr_token_filter(int token);
static void comment(void);
static int pragma_option_to_token(void);
void scanner_clean_until_new_line(void);

size_t position_in_file;
size_t position_of_last_starting_row;
size_t position_scanning_row;
size_t position_scanning_column;
size_t position_start_current_token;
size_t last_token_size;
size_t last_pragma_start_line;
static bool parsing_pragma_acr_name;

%}

%%
"/*"              {
                    comment();
                  }
"//".*            {
                    /* consume //-comment */
                    for (size_t i = 0; yytext[i] != '\0'; ++i) {
                      ++position_in_file;
                      ++position_scanning_column;
                    }
                  }

 /*
"auto"           { return acr_token_filter(AUTO); }
"break"          { return acr_token_filter(BREAK); }
"case"           { return acr_token_filter(CASE); }
"char"           { return acr_token_filter(CHAR); }
"const"          { return acr_token_filter(CONST); }
"continue"       { return acr_token_filter(CONTINUE); }
"default"        { return acr_token_filter(DEFAULT); }
"do"             { return acr_token_filter(DO); }
"double"         { return acr_token_filter(DOUBLE); }
"else"           { return acr_token_filter(ELSE); }
"enum"           { return acr_token_filter(ENUM); }
"extern"         { return acr_token_filter(EXTERN); }
"float"          { return acr_token_filter(FLOAT); }
"for"            { return acr_token_filter(FOR); }
"goto"           { return acr_token_filter(GOTO); }
"if"             { return acr_token_filter(IF); }
"inline"         { return acr_token_filter(INLINE); }
"int"            { return acr_token_filter(INT); }
"long"           { return acr_token_filter(LONG); }
"register"       { return acr_token_filter(REGISTER); }
"restrict"       { return acr_token_filter(RESTRICT); }
"return"         { return acr_token_filter(RETURN); }
"short"          { return acr_token_filter(SHORT); }
"signed"         { return acr_token_filter(SIGNED); }
"sizeof"         { return acr_token_filter(SIZEOF); }
"static"         { return acr_token_filter(STATIC); }
"struct"         { return acr_token_filter(STRUCT); }
"switch"         { return acr_token_filter(SWITCH); }
"typedef"        { return acr_token_filter(TYPEDEF); }
"union"          { return acr_token_filter(UNION); }
"unsigned"       { return acr_token_filter(UNSIGNED); }
"void"           { return acr_token_filter(VOID); }
"volatile"       { return acr_token_filter(VOLATILE); }
"while"          { return acr_token_filter(WHILE); }
"_Alignas"       { return acr_token_filter(ALIGNAS); }
"_Alignof"       { return acr_token_filter(ALIGNOF); }
"_Atomic"        { return acr_token_filter(ATOMIC); }
"_Bool"          { return acr_token_filter(BOOL); }
"_Complex"       { return acr_token_filter(COMPLEX); }
"_Generic"       { return acr_token_filter(GENERIC); }
"_Imaginary"     { return acr_token_filter(IMAGINARY); }
"_Noreturn"      { return acr_token_filter(NORETURN); }
"_Static_assert" { return acr_token_filter(STATIC_ASSERT); }
"_Thread_local"  { return acr_token_filter(THREAD_LOCAL); }
"__func__"       { return acr_token_filter(FUNC_NAME); }
*/

"#pragma"[ ]+"acr"  {
                      parsing_pragma_acr = true;
                      parsing_pragma_acr_name = true;
                      last_pragma_start_line = position_of_last_starting_row;
                      return acr_token_filter(PRAGMA_ACR);
                    }

{L}{A}*           {
                    yylval.identifier = yytext;
                    if (parsing_pragma_acr_name == true) {
                      parsing_pragma_acr_name = false;
                      return acr_token_filter(pragma_option_to_token());
                    }
                    return acr_token_filter(IDENTIFIER);
                  }

{HP}{H}+{IS}?               { return acr_token_filter(I_CONSTANT); }
{NZ}{D}*{IS}?               { return acr_token_filter(I_CONSTANT); }
"0"{O}*{IS}?                { return acr_token_filter(I_CONSTANT); }
{CP}?"'"([^'\\\n]|{ES})+"'" { return acr_token_filter(I_CONSTANT); }

{D}+{E}{FS}?            { return acr_token_filter(F_CONSTANT); }
{D}*"."{D}+{E}?{FS}?    { return acr_token_filter(F_CONSTANT); }
{D}+"."{E}?{FS}?        { return acr_token_filter(F_CONSTANT); }
{HP}{H}+{P}{FS}?        { return acr_token_filter(F_CONSTANT); }
{HP}{H}*"."{H}+{P}{FS}? { return acr_token_filter(F_CONSTANT); }
{HP}{H}+"."{P}{FS}?     { return acr_token_filter(F_CONSTANT); }

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+ {
                                    return acr_token_filter(STRING_LITERAL);
                                  }

"..."       { return acr_token_filter(ELLIPSIS); }
">>="       { return acr_token_filter(RIGHT_ASSIGN); }
"<<="       { return acr_token_filter(LEFT_ASSIGN); }
"+="        { return acr_token_filter(ADD_ASSIGN); }
"-="        { return acr_token_filter(SUB_ASSIGN); }
"*="        { return acr_token_filter(MUL_ASSIGN); }
"/="        { return acr_token_filter(DIV_ASSIGN); }
"%="        { return acr_token_filter(MOD_ASSIGN); }
"&="        { return acr_token_filter(AND_ASSIGN); }
"^="        { return acr_token_filter(XOR_ASSIGN); }
"|="        { return acr_token_filter(OR_ASSIGN); }
">>"        { return acr_token_filter(RIGHT_OP); }
"<<"        { return acr_token_filter(LEFT_OP); }
"++"        { return acr_token_filter(INC_OP); }
"--"        { return acr_token_filter(DEC_OP); }
"->"        { return acr_token_filter(PTR_OP); }
"&&"        { return acr_token_filter(AND_OP); }
"||"        { return acr_token_filter(OR_OP); }
"<="        { return acr_token_filter(LE_OP); }
">="        { return acr_token_filter(GE_OP); }
"=="        { return acr_token_filter(EQ_OP); }
"!="        { return acr_token_filter(NE_OP); }
";"         { return acr_token_filter(';'); }
("{"|"<%")  { return acr_token_filter('{'); }
("}"|"%>")  { return acr_token_filter('}'); }
","         { return acr_token_filter(','); }
":"         { return acr_token_filter(':'); }
"="         { return acr_token_filter('='); }
"("         { return acr_token_filter('('); }
")"         { return acr_token_filter(')'); }
("["|"<:")  { return acr_token_filter('['); }
("]"|":>")  { return acr_token_filter(']'); }
"."         { return acr_token_filter('.'); }
"&"         { return acr_token_filter('&'); }
"!"         { return acr_token_filter('!'); }
"~"         { return acr_token_filter('~'); }
"-"         { return acr_token_filter('-'); }
"+"         { return acr_token_filter('+'); }
"*"         { return acr_token_filter('*'); }
"/"         { return acr_token_filter('/'); }
"%"         { return acr_token_filter('%'); }
"<"         { return acr_token_filter('<'); }
">"         { return acr_token_filter('>'); }
"^"         { return acr_token_filter('^'); }
"|"         { return acr_token_filter('|'); }
"?"         { return acr_token_filter('?'); }

"\\\n"    {
            position_in_file += 2;
            position_of_last_starting_row = position_in_file;
            ++position_scanning_row;
            position_scanning_column = 0;
          }

{WS}+     {
            /* whitespace separates tokens */
            for(size_t i = 0; yytext[i] != '\0'; ++i) {
              ++position_in_file;
              if (yytext[i] == '\n') {
                parsing_pragma_acr = false;
                position_of_last_starting_row = position_in_file;
                ++position_scanning_row;
                position_scanning_column = 0;
              }
              else
                ++position_scanning_column;
            }
          }

.         {
            /* discard bad characters */
            ++position_scanning_column;
            ++position_in_file;
          }

%%

int yywrap(void) {    /* called at end of input */
  return 1;           /* terminate now */
}

static void update_scanner_positions(void) {
  last_token_size = 0;
  position_start_current_token = position_scanning_column;
  position_scanning_column += yyleng;
  position_in_file += yyleng;
  last_token_size = yyleng;
}

int acr_token_filter(int token) {
  update_scanner_positions();
  return parsing_pragma_acr ? token : IGNORE;
}

static void comment_error(size_t position_row_start_of_string,
                          size_t row_start_of_string) {
  position_of_last_starting_row = position_row_start_of_string;
  position_scanning_column      = position_start_current_token + 1;
  position_scanning_row         = row_start_of_string;
  last_token_size = 1;
  yyerror("[ACR] Error: Unterminated string");
}

static void comment(void) {
  int c;
  size_t start_of_row            = position_of_last_starting_row;
  size_t comment_starting_row    = position_scanning_row;

  position_start_current_token = position_scanning_column;
  position_in_file += 2;
  position_scanning_column += 2;


  while ((c = input()) != EOF) {
    ++position_in_file;

    if (c == '\n') {
      position_of_last_starting_row = position_in_file;
      ++position_scanning_row;
      position_scanning_column = 0;
      continue;
    }

    ++position_scanning_column;

    if (c == '*') {
      while ((c = input()) == '*') {
        ++position_in_file;
        ++position_scanning_column;
      }

      ++position_in_file;

      if (c == '/') {
        ++position_scanning_column;
        return;
      }

      if (c == '\n') {
        position_of_last_starting_row = position_in_file;
        ++position_scanning_row;
        position_scanning_column = 0;
        continue;
      }

      if (c == EOF)
        break;
    }
  }
  comment_error(start_of_row, comment_starting_row);
}

static int pragma_option_to_token(void) {
  for(int i = acr_type_alternative; i < acr_type_unknown; ++i) {
    if (strcmp(yytext, acr_pragma_options_name_list[i].name) == 0)
      return acr_pragma_options_name_list[i].token_id;
  }
  return ACR_UNKNOWN; // parse error
}

void scanner_clean_until_new_line(void) {
  int c;
  int previous;

  c = input();

  if(c == EOF)
    return;

  if (c == '\n') {
    ++position_in_file;
    position_of_last_starting_row = position_in_file;
    ++position_scanning_row;
    position_scanning_column = 0;
    return;
  } else {
    ++position_in_file;
    ++position_scanning_column;
  }

  while(previous = c, (c = input()) != EOF) {
    if (previous == '\\' && c == '\n') {
      position_in_file += 2;
      position_of_last_starting_row = position_in_file;
      ++position_scanning_row;
      position_scanning_column = 0;
    } else {
      if(c == '\n') {
        ++position_in_file;
        position_of_last_starting_row = position_in_file;
        ++position_scanning_row;
        position_scanning_column = 0;
        return;
      } else {
        ++position_in_file;
        ++position_scanning_column;
      }
    }
  }
}

int main() {
  position_in_file = 0;
  position_of_last_starting_row = 0;
  position_scanning_row = 0;
  position_scanning_column = 0;
  position_start_current_token = 0;
  last_token_size = 0;
  parsing_pragma_acr_name = false;
  printf("opening %s\n", current_file);
  yyin = fopen(current_file, "r");
  if (yyin == NULL)
    printf("WTF NULL\n");
  return yyparse();
}
