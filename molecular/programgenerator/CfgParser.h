#pragma once
#include <string>
#include "ProgramFile.h"
#include "GlslTranslator.tab.hpp"

class ShaderParser
{
private:
	void* scanner;
	void* buffer;
	bool Parse(molecular::programgenerator::ProgramFile& progfile);
public:
	static bool Parse(const std::string& inp_str,
					  molecular::programgenerator::ProgramFile& progfile);
//	int GetNextToken(std::string& tok_val);
	ShaderParser(const std::string& inp_str);
	~ShaderParser();

	ShaderParser() = delete;
	ShaderParser(const ShaderParser& obj) = delete;
	const ShaderParser& operator=(const ShaderParser& obj) = delete;
};
