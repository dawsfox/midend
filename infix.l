%{
#include <stdlib.h> /* for atoi call */
#include <string.h> /* for strcmp */
/* #include "cal.tab.h" */

#define DEBUG /* for debuging: print tokens and their line numbers */
#define NUMBER 258 /* copy this from cal.tab.c */
#define VAR 259 /* copied from infix.tab.c */
#define POW 260 /* copied from infix.tab.c */

typedef union { /* copy this from cal.tab.c */
int d;
char text[36];
} YYSTYPE;

YYSTYPE yylval; /* for passing value to parser */
extern int lineNum; /* line number from cal.tab.c */
extern int left_paren;
extern int right_paren;

%}

%%
[ \t]+ {}

[\n] { 
	lineNum++; 
	#ifdef DEBUG
	printf("token 'newline' at line %d\n", lineNum);
	#endif
	return '\n';
	}

";" {
	#ifdef DEBUG
	printf("token ';' at line %d\n", lineNum);
	#endif
	return ';';
	}

"?" {
	#ifdef DEBUG
	printf("token '?' at line %d\n", lineNum);
	#endif
	return '?';
	}

"(" {
	left_paren++;
	#ifdef DEBUG
	printf("token '(' at line %d\n", lineNum);
	#endif
	return '(';
	}

")" {
	right_paren++;
	#ifdef DEBUG
	printf("token ')' at line %d\n", lineNum);
	#endif
	return ')';
	}

"+" {
	#ifdef DEBUG
	printf("token '+' at line %d\n", lineNum);
	#endif
	return '+';
	}

"-" {
	#ifdef DEBUG
	printf("token '-' at line %d\n", lineNum);
	#endif
	return '-';
	}

"*" {
	#ifdef DEBUG
	printf("token '*' at line %d\n", lineNum);
	#endif
	return '*';
	}

"/" {
	#ifdef DEBUG
	printf("token '/' at line %d\n", lineNum);
	#endif
	return '/';
	}

[0-9]+ {
	#ifdef DEBUG
	printf("token %s at line %d\n", yytext, lineNum);
	#endif
	//yylval.d = atoi(yytext);
	strcpy(yylval.text, yytext);
	return NUMBER;
	}

"=" {
	#ifdef DEBUG
	printf("token '=' at line %d\n", lineNum);
	#endif
	return '=';
	}

[a-z]+[a-zA-Z0-9_]* {
	#ifdef DEBUG
	printf("token %s at line %d\n", yytext, lineNum);
	#endif
	strcpy(yylval.text, yytext);
	return VAR;
	}

"!" {
	#ifdef DEBUG
	printf("token '!' at line %d\n", lineNum);
	#endif
	return '!';
	}

"**" {
	#ifdef DEBUG
	printf("token 'POW' at line %d\n", lineNum);
	#endif
	return POW;
	}
%%
int yywrap() { /* need this to avoid link problem */
	return 1;
}
