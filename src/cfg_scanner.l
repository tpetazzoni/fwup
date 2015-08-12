%x comment str

DEC         [0-9]
NONZERO     [1-9]
HEX         [0-9a-fA-F]
OCT         [0-7]
ALPHA       [a-zA-Z_]
ALPHANUM    [a-zA-Z_0-9]
WHITESPACE  [ \t\v\f]

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "parser.h"

int lineno = 1;

extern void yyerror(const char *);
static char string_buf[1024];
static char *string_buf_ptr;

static char *resolve_env_var()
{
    char *var = &yytext[2];
    var[strlen(var) - 1] = 0;
    char *value = getenv(var);
    return value ? value : "";
}

%}

%option 8bit
%option nounput nomain noyywrap
%option warn

%%

"/*"         BEGIN(comment);
<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>\n             ++lineno;
<comment>"*"+"/"        BEGIN(INITIAL);

"//".*      { /* ignore // comment */ }
#.*         { /* ignore # comment */ }


-?{NONZERO}{DEC}*{ALPHA}*  { yylval.string = strdup(yytext); return RHS_ONLY_TERM; }
-?0x{HEX}+{ALPHA}*         { yylval.string = strdup(yytext); return RHS_ONLY_TERM; }
-?0{OCT}+{ALPHA}*          { yylval.string = strdup(yytext); return RHS_ONLY_TERM; }

{ALPHA}{ALPHANUM}*         { yylval.string = strdup(yytext); return TERM; }
"${"{ALPHA}{ALPHANUM}*"}"  { yylval.string = strdup(resolve_env_var()); return RHS_ONLY_TERM; }

\"      string_buf_ptr = string_buf; BEGIN(str);

<str>{
    \"        { /* saw closing quote - all done */
                 BEGIN(INITIAL);
                 *string_buf_ptr = '\0';
                 yylval.string = strdup(string_buf);
                 return RHS_ONLY_TERM;
                 }

    \n        {
                 yyerror("unterminated string constant");
                 }

    \\[0-7]{1,3} {
                 /* octal escape sequence */
                 int result = strtol(yytext, 0, 8);
                 if (result > 0xff)
                   yyerror("octal constant out of bounds");

                 *string_buf_ptr++ = result;
                 }

    \\[0-9]+ {
                 /* generate error - bad escape sequence; something
                  * like '\48' or '\0777777'
                  */
                 yyerror("bad escape sequence");
                 }

    \\n  *string_buf_ptr++ = '\n';
    \\t  *string_buf_ptr++ = '\t';
    \\r  *string_buf_ptr++ = '\r';
    \\b  *string_buf_ptr++ = '\b';
    \\f  *string_buf_ptr++ = '\f';
    \$   *string_buf_ptr++ = '$'; /* flex will match the longest sequence, so env vars take precedence. */

    \\(.|\n)  *string_buf_ptr++ = yytext[1];

    "${"{ALPHA}{ALPHANUM}*"}" { strcat(string_buf_ptr, resolve_env_var()); string_buf_ptr += strlen(string_buf_ptr); }


    [^$\\\n\"]+        {
                 char *yptr = yytext;

                 while ( *yptr )
                         *string_buf_ptr++ = *yptr++;
                 }
}

"="         { return '='; }
"{"         { return '{'; }
"}"         { return '}'; }
"("         { return '('; }
")"         { return ')'; }
","         { return ','; }

";"         { /* ignore ';', since I keep on typing them. */ }

{WHITESPACE} { /* ignore whitespace */ }
\r?\n       { lineno++; }
.           { /* ignore the rest */ }

%%

