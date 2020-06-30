%option prefix="preposit_formula_"
%option noyywrap
%option reentrant
%option bison-bridge
%option yylineno
%option bison-locations


%{
#include <iostream>
#include <string>
#include <algorithm>
#include "GlslTranslator.tab.hpp"
#ifdef YYDEBUG
#define prdbg(msg, args...) fprintf(stderr, "DEBUG: " msg "\n", ##args)
#else
#define prdbg(msg, args...)
#endif

#define YY_USER_ACTION yylloc->first_column += strlen(yytext);
using namespace std;
%}

%x beginsrc
%%

"\n" {
    //count lines, but ignore newlines
    yylloc->first_line++;
    yylloc->first_column = 0;
}


"@src" {
    BEGIN(beginsrc);
}
<beginsrc>.|\n {
        yymore();
}

<beginsrc>"@endsrc" {
        *yylval = string(yytext);
        yylval->erase(yylval->size() - sizeof("@endsrc") + 1,
                      sizeof("@endsrc") - 1);
        yylloc->first_line += std::count(yylval->begin(), yylval->end(), '\n');
        prdbg("recognised source: %s", yylval->c_str());
        BEGIN(INITIAL);
        return source;
}

[\ \t] {
    //ignore delimiters
}

^"//".* {
    //ignore comment
}

"attr"[0-9]* {
    *yylval = string(yytext + 4);
    prdbg("recognised attr: %s", yylval->c_str());
    return attr;
}

"out" {
    return out;
}

"vertex" {
    prdbg("recognised vertex");
    return vertex;
}

"fragment" {
    prdbg("recognised fragment ");
    return fragment;
}

"prio="[0-9]+ {
    *yylval = string(yytext + 5);
    prdbg("recognised prio: %s", yylval->c_str());
    return prio;
}

[a-zA-Z_]+([a-zA-Z_]*[0-9]*)* {
    *yylval = string(yytext);
    prdbg("recognised identifier: %s", yylval->c_str());
    return identifier;
}

. {
    prdbg("recognised symbol: %c", yytext[0]);
    return yytext[0];
}

%%
