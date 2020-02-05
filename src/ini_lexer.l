%option debug
%option yylineno
%option noyywrap
%option reentrant
%option bison-bridge
%option header-file="ini_lexer.h"

%{
#include "ini_parser.h"
#include "ini_file.h"
#include "config.h"
#ifdef DEBUG
#include <stdbool.h>
#endif
%}

%%

[\[] {
	if (DEBUG) {
		printf("(%u:lbracket)", yylineno);
	}
	return TOKEN_LBRACKET;
}

[\]] {
	if (DEBUG) {
		printf("(%u:lbracket)", yylineno);
	}
	return TOKEN_RBRACKET;
}

[=] {
	if (DEBUG) {
		printf("(%u:equals)", yylineno);
	}
	return TOKEN_EQUALS;
}

;.* {
	if (DEBUG) {
		printf("(%u:comment)", yylineno);
	}
}

[\n] {
	if (DEBUG) {
		printf("\n");
	}
}

\"([^"\\]|\\.)*\" {
	if (DEBUG) {
		printf("(%u:string<%s>)", yylineno, yytext);
	}
	yylval->str = strndup(yytext, MAX_FIELD_LENGTH);
	return TOKEN_STRING;
} /*"*/

[^[:space:]=;\[\]]([^=;\n]*[^[:space:];=\[\]])* {
	if (DEBUG) {
		printf("(%u:string<%s>)", yylineno, yytext);
	}
	yylval->str = strndup(yytext, MAX_FIELD_LENGTH);
	return TOKEN_STRING;
}

[[:space:]] {
	if (DEBUG) {
		printf("(%u:whitespace)", yylineno);
	}
}

<<EOF>>	{
	if (DEBUG) {
		printf("(eof %u)\n", yylineno);
	}
	return 0;
}

%%
