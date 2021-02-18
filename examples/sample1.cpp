#include <vector>
#include <string>
#include <iostream>
#include <regex>
#include <fstream>
#include <sstream>

#include <molecular/programgenerator/ProgramGenerator.h>
#include <molecular/programgenerator/ProgramFile.h>
#include <molecular/util/CommandLineParser.h>

using namespace molecular::util;

int main(int argc, char** argv)
{
	
	CommandLineParser cmd;
	CommandLineParser::Option<std::string> inp(cmd, "inputs", "input shader variables");
	CommandLineParser::Option<std::string> out(cmd, "outputs", "output variables for shader");
	CommandLineParser::Option<std::string> file(cmd, "shader-file", "shader source file", "");
	CommandLineParser::HelpFlag help(cmd);

	cmd.Parse(argc, argv);
	std::regex inpOutRegex("[a-zA-Z_]+([a-zA-Z_]*[0-9]*)*");
	std::cout << "argc: " << argc<< std::endl;

	std::cout << *inp << std::endl;
	std::string inputsString = *inp;
	auto inputs_begin = std::sregex_iterator(inputsString.begin(), inputsString.end(), inpOutRegex);
	auto inputs_end = std::sregex_iterator();
	std::vector<std::string> inputs;
	for(std::sregex_iterator it = inputs_begin; it != inputs_end; it++)
	{
		inputs.push_back(it->str());
		std::cout << "input: " << inputs.back() << std::endl;
	}
	
	std::string outputString = *out;
	auto out_begin = std::sregex_iterator(outputString.begin(), outputString.end(), inpOutRegex);
	auto out_end = std::sregex_iterator();
	std::vector<std::string> outputs;
	for(auto it = out_begin; it != out_end; it++)
		outputs.push_back(it->str());
	
	std::ifstream glslFile(*file, std::ios::in);
	if(!glslFile.is_open())
	{
		std::cout << "failed to open file " << *file;
		return -1;
	}
	
	std::stringstream ss;
	ss << glslFile.rdbuf();
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(ss.str().size());
	std::memcpy(buffer.get(), ss.str().c_str(), ss.str().size());
	
	molecular::programgenerator::ProgramFile programFile(buffer.get(), buffer.get() + ss.str().size());
	molecular::programgenerator::ProgramGenerator generator;

	for(auto& variable: programFile.GetVariables())
		generator.AddVariable(variable);
	
	for(auto& function: programFile.GetFunctions())
		generator.AddFunction(function);
	
	std::vector<molecular::util::Hash> variables;
	for(const auto& in : inputs)
		variables.push_back(molecular::util::HashUtils::MakeHash(in));
	for(const auto& out : outputs)
		variables.push_back(molecular::util::HashUtils::MakeHash(out));
	
	molecular::programgenerator::ProgramGenerator::ProgramText glslProgramText = generator.GenerateProgram(variables.begin(), variables.end());
	
	std::cout << glslProgramText.vertexShader << std::endl;
	std::cout << glslProgramText.fragmentShader << std::endl;
	std::cout << glslProgramText.geometryShader << std::endl;

}