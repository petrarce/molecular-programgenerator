#include <cassert>
#include "CfgParser.h"
#include "GlslTranslator.tab.hpp"
#include "GlslTranslator.lex.hpp"

using namespace std;

bool ShaderParser::Parse(molecular::programgenerator::ProgramFile& progfile)
{
    if(preposit_formula_parse((yyscan_t)this->scanner, progfile)){
    	return false;
    }
	return true;
}

bool ShaderParser::Parse(const string& inp_str,
						   molecular::programgenerator::ProgramFile& progfile)
{
	ShaderParser parser(inp_str);
	return parser.Parse(progfile);
}
//int ShaderParser::GetNextToken(string& tok_val)
//{
//	return preposit_formula_lex(&tok_val, this->scanner);
//}
ShaderParser::ShaderParser(const string& inp_str):
    scanner(NULL),
    buffer(NULL)
{
    if(preposit_formula_lex_init(&this->scanner)){
        assert(0 && "couldnt allocate memory for new scanner in the parser");
    }
    this->buffer = preposit_formula__scan_string(inp_str.c_str(), this->scanner);
    if(this->buffer == NULL){
        assert(0 && "failed to allocate buffer for parsing");
    }
}
ShaderParser::~ShaderParser()
{
    preposit_formula__delete_buffer((YY_BUFFER_STATE)this->buffer, this->scanner);
    preposit_formula_lex_destroy(this->scanner);
}
