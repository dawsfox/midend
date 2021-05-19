%{
#include <stdio.h>
#include <ctype.h>
int lineNum = 1;
void yyerror(char *ps, ...) { /* need this to avoid link problem */
	printf("%s\n", ps);
}
%}

%union {
int d;
}

FILE *yyin;

// need to choose token type from union above
%token <d> NUMBER
%token '(' ')'
%left '+' '-'
%left '*' '/'
%right '!'
%type <d> exp factor term
%start cal


%%
cal : exp
	{ printf("The result is %d\n", $1); }
	;

exp : exp '+' factor
	{ $$ = $1 + $3; }
    | factor
	{ $$ = $1; }
	;

factor : factor '*' term
	{ $$ = $1 * $3; }
       | term
	{ $$ = $1; }
	;

term : NUMBER
	{ $$ = $1; }
     | '(' exp ')'
	{ $$ = $2; }
	;

%%
int main() {
	yyin = fopen(argv[1], "r");
	yyparse();
	fclose(yyin);
	return 0;
}
