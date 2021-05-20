%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
int lineNum = 1;
int left_paren = 0;
int right_paren = 0;
int sym[10];
//char prefix[80];
char tmp[80];
char tmp_assign1[30];
char tmp_assign2[30];
char tertiary_exp[16];
char **symList;
int latest = -1;
int op_used = 0;
int curr_tmp = 0;
int tertiary = 0;
int tertiary_mark = 0;

FILE *yyin;
FILE *output;

int yylex();

int checkSym(char **list, char *sym) {
        for (int i=0; i<latest; i++) {
                //match
                if (strcmp(sym, list[i]) == 0) {
                        return i;
                }
        }
	if (latest < 0) {
		latest = 0;
	}
        // no match
        list[latest] = (char *)malloc(strlen(sym) + 1); //account for null char
        strcpy(list[latest], sym); // add to list
        latest++; //increment storing index
        return latest-1; //return location of sym after added/incremented
}


void yyerror(char *ps, ...) { /* need this to avoid link problem */
	printf("%s\n", ps);
	for (int i=0; i<10; i++) {
		free(symList[i]);
	}
	free(symList);
	if (left_paren != right_paren) {
		printf("unmatched parentheses detected\n");
	}
}
%}

%union {
int d;
char text[36];
}

// need to choose token type from union above
%token <text> NUMBER
%token <text> VAR
%token '(' ')' '\n'
%left '+' '-'
%left '*' '/'
%left POW
%left '?'
%right '!'
%type <text> command exp factor exponent term
%start program


%%

program : program command
	| command
	;

command : exp '\n'
	{ //printf("%s\n", prefix);
	  //print prefix expression
	  //prefix[0] = '\0';
	  //printf("=%d\n", $1); 
	  op_used = 0;
	}
	;

exp : VAR '=' exp
	{ //sym[checkSym(symList, $1)] = $3;
	  if (tertiary) {
		tertiary = 0;
		fprintf(output, "\t%s\n", tmp_assign1);
		fprintf(output, "\t%s = %s\n", $1, tertiary_exp);
		fprintf(output, "} else {\n");
		fprintf(output, "\t%s = 0\n", $1);
		fprintf(output, "}\n");
	  }
	  if (tertiary_mark) {
		tertiary_mark = 0;
		sprintf(tmp_assign1, "%s = %s", $1, $3);
	  }
	  else {
		  if (op_used > 1) {
			fprintf(output, "%s = %s\n", $1, tmp);
			sprintf(tmp, "%s", $1);
		  }
		  else if (op_used == 1) {
			fprintf(output, "%s = %s\n", $1, tmp);
			sprintf(tmp, "%s", $1);
		  }
		  else {
			fprintf(output, "%s = %s\n", $1, $3);
	  	}
	  }
	  strcpy(tmp_assign2, tmp_assign1);
	  sprintf(tmp_assign1, "%s = %s", $1, $3);
	  strcpy($$, $1);
	}
    | exp tert exp
	{ /* if ($1 == 0) {
		$$ = 0;
	  }
	  else {
		$$ = $3;
	  }
	  fprintf(output, "If(%s){\n", $1);
	  fprintf(output, "\t%s\n", $3);
	  fprintf(output, "} else {\n");
	  fprintf(output, "\t
	  */
	  tertiary = 1;
	  strcpy(tertiary_exp, $3);
	  fprintf(output, "If(%s){\n", $1);
	  
	}
    | exp '+' exp
	{ //$$ = $1 + $3; 
	  if (!op_used) {
		op_used = 1;
		sprintf($$, "tmp%d", curr_tmp);
		sprintf(tmp, "%s+%s", $1, $3);
	  }
	  else {
		fprintf(output, "tmp%d = %s\n", curr_tmp, tmp);
		sprintf(tmp, "%s+%s", $1, $3);
		curr_tmp++;
		sprintf($$, "tmp%d", curr_tmp);
		op_used++;
	  }
	}
    | exp '-' exp
	{ //$$ = $1 - $3; 
	  if (!op_used) {
		op_used = 1;
		sprintf($$, "tmp%d", curr_tmp);
		sprintf(tmp, "%s-%s", $1, $3);
	  }
	  else {
		fprintf(output, "tmp%d = %s\n", curr_tmp, tmp);
		sprintf(tmp, "%s-%s", $1, $3);
		curr_tmp++;
		sprintf($$, "tmp%d", curr_tmp);
		op_used++;
	  }
	}
    | factor
	{ //$$ = $1; 
	  strcpy($$, $1);
	}
	;

factor : factor '*' factor
	{ //$$ = $1 * $3; 
	  if (!op_used) {
		op_used = 1;
		sprintf($$, "tmp%d", curr_tmp);
		sprintf(tmp, "%s*%s", $1, $3);
	  }
	  else {
		fprintf(output, "tmp%d = %s\n", curr_tmp, tmp);
		sprintf(tmp, "%s*%s", $1, $3);
		curr_tmp++;
		sprintf($$, "tmp%d", curr_tmp);
		op_used++;
	  }
	}
       | factor '/' factor
	{ //$$ = $1 / $3; 
	  if (!op_used) {
		op_used = 1;
		sprintf($$, "tmp%d", curr_tmp);
		sprintf(tmp, "%s/%s", $1, $3);
	  }
	  else {
		fprintf(output, "tmp%d = %s\n", curr_tmp, tmp);
		sprintf(tmp, "%s/%s", $1, $3);
		curr_tmp++;
		sprintf($$, "tmp%d", curr_tmp);
		op_used++;
	  }
	}
       | exponent 
	{ //$$ = $1; 
	  strcpy($$, $1);
	}
	;

exponent : term POW term
	{ //$$ = (int) pow($1, $3); 
	  if (!op_used) {
	  	op_used = 1;
	  	sprintf($$, "tmp%d", curr_tmp);
		sprintf(tmp, "%s**%s", $1, $3);
	  }
	  else {
		fprintf(output, "tmp%d = %s\n", curr_tmp, tmp);
		sprintf(tmp, "%s**%s", $1, $3);
		curr_tmp++;
		sprintf($$, "tmp%d", curr_tmp);
		op_used++;
	  }
	}
	 | '!' term
	{ //$$ = !$2; 
	  if (!op_used) {
	  	op_used = 1;
		sprintf($$, "tmp%d", curr_tmp);
	  	sprintf(tmp, "!%s", $2); 
	  }
	  else {
	 	fprintf(output, "tmp%d = %s\n", curr_tmp, tmp); 
		sprintf(tmp, "!%s", $2);
		curr_tmp++;
		sprintf($$, "tmp%d", curr_tmp);
		op_used++;
	  }
	}
         | term
	{ //$$ = $1; 
	  strcpy($$, $1);
	}
	;

term : NUMBER
	{ //$$ = $1; 
	  strcpy($$, $1);
	}
     | VAR
	{ //$$ = sym[checkSym(symList, $1)];
	  //$$ = $1;
	  strcpy($$, $1);
	}
     | '(' exp ')'
	{ //$$ = $2; 
	  strcpy($$, $2);
	}
	;

tert : '?'
	{
	  tertiary_mark = 1;
	}

%%
int main(int argc, char *argv[]) {
	symList = (char **)malloc(10 * sizeof(char *)); //list of dynamically allocated arrays (char pointers)
	yyin = fopen(argv[1], "r");
	output = fopen(argv[2], "w");
	yyparse();
	fclose(yyin);
	for (int i=latest; i>=0; i--) {
		free(symList[i]);
	}
	free(symList);
	return 0;
}
