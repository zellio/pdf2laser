%{
#include "ini_parser.h"
#include "ini_lexer.h"  // for yy_delete_buffer, yy_scan_string, yylex_destroy, yylex_init, YY_BUFFER_STATE, yyscan_t

extern int yylex();
void yyerror(void *scanner, const char *s);

ini_file_t *ini_file;
%}

%code requires {
#include "ini_file.h"
}

%code provides {
	int ini_file_parse(char *text, ini_file_t **ini_file);
}

%debug
%define api.pure
%lex-param {void *scanner}
%parse-param {void *scanner}

%union {
	char *str;
	ini_entry_t *entry;
	ini_section_t *section;
	ini_file_t *file;
}

%token  <token>         TOKEN_LBRACKET TOKEN_RBRACKET TOKEN_WHITESPACE TOKEN_EQUALS
%token  <str>           TOKEN_COMMENT TOKEN_STRING

%type   <entry>         entries entry
%type   <section>       sections section

%start file

%%

file:
                sections
                { ini_file = ini_file_create("");
                  ini_file->sections = $sections; }
        ;

sections:
                section { $$ = $section; $$->next = NULL; }
        |       section sections { $$ = $section; $$->next = $2; }
        ;

section:
                TOKEN_LBRACKET TOKEN_STRING[name] TOKEN_RBRACKET entries
                { $$ = ini_section_create($name);
                  $$->entries = $entries; }
        ;

entries:
                entry { $$ = $entry; $$->next = NULL; }
        |       entry entries { $$ = $entry; $$->next = $2; }
        ;

entry:
                TOKEN_STRING[key] TOKEN_EQUALS TOKEN_STRING[value]
                { $$ = ini_entry_create($key, $value); }
        ;

%%

void yyerror(__attribute__ ((unused)) void *scanner, const char *s)
{
	printf("ERROR: %s\n", s);
}


int ini_file_parse(char *text, ini_file_t **file)
{
	yyscan_t scanner;
	yylex_init(&scanner);

	YY_BUFFER_STATE buffer = yy_scan_string(text, scanner);
    int parse_rc = yyparse(scanner);

	yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);

	if (!parse_rc) {
		*file = ini_file;
		return 0;
	}

	return -1;
}
