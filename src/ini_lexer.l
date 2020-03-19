%option debug
%option yylineno
%option noyywrap
%option reentrant
%option bison-bridge
%option header-file="ini_lexer.h"

%{
#include "ini_parser.h"  // for YYSTYPE, TOKEN_STRING, TOKEN_EQUALS, TOKEN_LBRACKET, TOKEN_RBRACKET
#include "ini_file.h"    // for MAX_FIELD_LENGTH
#include "config.h"      // for DEBUG_PARSER
#ifdef DEBUG_PARSER
#include <stdbool.h>
#endif
%}

%%

[\[] {
	if (DEBUG_PARSER) {
		printf("(%u:lbracket)", yylineno);
	}
	return TOKEN_LBRACKET;
}

[\]] {
	if (DEBUG_PARSER) {
		printf("(%u:rbracket)", yylineno);
	}
	return TOKEN_RBRACKET;
}

[=] {
	if (DEBUG_PARSER) {
		printf("(%u:equals)", yylineno);
	}
	return TOKEN_EQUALS;
}

;.* {
	if (DEBUG_PARSER) {
		printf("(%u:comment)", yylineno);
	}
}

[\n] {
	if (DEBUG_PARSER) {
		printf("\n");
	}
}

\"([^"\\]|\\.)*\" {
	if (DEBUG_PARSER) {
		printf("(%u:string<%s>)", yylineno, yytext);
	}
	yylval->str = strndup(yytext, MAX_FIELD_LENGTH);
	return TOKEN_STRING;
} /*"*/

[^[:space:]=;\[\]]([^=;\n]*[^[:space:];=\[\]])* {
	if (DEBUG_PARSER) {
		printf("(%u:string<%s>)", yylineno, yytext);
	}
	yylval->str = strndup(yytext, MAX_FIELD_LENGTH);
	return TOKEN_STRING;
}

[[:space:]] {
	if (DEBUG_PARSER) {
		printf("(%u:whitespace)", yylineno);
	}
}

<<EOF>>	{
	if (DEBUG_PARSER) {
		printf("(eof %u)\n", yylineno);
	}
	return 0;
}

%%
