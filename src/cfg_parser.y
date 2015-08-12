%{
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "cfgnode.h"

void yyerror(struct cfg_node **cfg, const char *msg);
int yylex();
extern int lineno;

%}

%union {
        char *string;
        struct cfg_node *node;
}

%token <string> RHS_ONLY_TERM    /* Numbers, strings, things from the environment */
%token <string> TERM

%type <node> element
%type <node> list
%type <node> function_call
%type <node> block
%type <node> assignment
%type <node> statement
%type <node> statements
%type <node> cfgfile

%parse-param {struct cfg_node **cfg}

%debug

%%

cfgfile    : statements { *cfg = reverse_cfg_nodes($1); }
           ;

statements : /* empty */ { $$ = NULL; }
           | statements statement { $2->next = $1; $$ = $2; }
           ;

statement : assignment
           | block
           | function_call
           ;

block      : TERM '{' statements '}' { $$ = create_cfg_block($1, NULL, reverse_cfg_nodes($3)); }
           | TERM TERM '{' statements '}' { $$ = create_cfg_block($1, $2, reverse_cfg_nodes($4)); }
           ;

assignment : TERM '=' element { $$ = create_cfg_assignment($1, $3); }
           | TERM '=' '{' list '}' { $$ = create_cfg_assignment($1, reverse_cfg_nodes($4)); }
           ;

function_call : TERM '(' list ')' { struct cfg_node *n = create_cfg_function($1, reverse_cfg_nodes($3));
                                    if ((strcmp(n->fun.name, "define") == 0 ||
                                         strcmp(n->fun.name, "define!") == 0) &&
                                        n->fun.args != NULL &&
                                        n->fun.args->next != NULL) {
                                        // Handle define(name, value) while parsing since they can affect
                                        // later lines.
                                        // NOTE: define!() overrides the environment; define() doesn't.
                                        if (setenv(n->fun.args->element.value,
                                                   n->fun.args->next->element.value,
                                                   strcmp(n->fun.name, "define!") == 0) < 0) {
                                            yyerror(cfg, "setenv failed");
                                            YYABORT;
                                        }

                                        free_cfg_node(n);
                                        $$ = create_cfg_empty();
                                    } else {
                                        $$ = n;
                                    }
                                  }
           ;

list       : element
           | list ',' element { $3->next = $1; $$ = $3; }
           ;

element    : TERM  { $$ = create_cfg_element($1); }
           | RHS_ONLY_TERM { $$ = create_cfg_element($1); }

%%

void yyerror(struct cfg_node **cfg, const char *str)
{
    fprintf(stderr, "yyerror:%d %s\n", lineno, str);
    *cfg = NULL;
}

int yywrap()
{
    return 1;
}

