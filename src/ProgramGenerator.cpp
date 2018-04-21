/*	ProgramGenerator.cpp

MIT License

Copyright (c) 2018 Fabian Herb

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ProgramGenerator.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

#ifndef LOG
#include <iostream>
#define LOG(x) std::cerr
#endif

std::string EmitGlslDeclaration(
		Hash variable,
		const ProgramGenerator::VariableInfo& info,
		const std::unordered_map<ProgramGenerator::Variable, int>& arraySizes)
{
	std::ostringstream oss;
	oss << info.type << " " << info.name;
	if(info.array)
		oss << "[" << arraySizes.at(variable) << "];\n";
	else
		oss << ";\n";
	return oss.str();
}

struct ProgramEmitterInput
{
	std::string vertexCode;
	std::string fragmentCode;
	std::unordered_set<ProgramGenerator::Variable> vertexInputs;
	std::unordered_set<ProgramGenerator::Variable> vertexLocals;
	std::unordered_set<ProgramGenerator::Variable> fragmentUniforms;
	std::unordered_set<ProgramGenerator::Variable> fragmentLocals;
	std::unordered_set<ProgramGenerator::Variable> fragmentAttributes;
};

ProgramGenerator::ProgramText EmitGlslProgram(
		const ProgramEmitterInput& input,
		const std::set<ProgramGenerator::Variable>& outputs,
		const std::unordered_map<ProgramGenerator::Variable, int>& arraySizes,
		const std::unordered_map<ProgramGenerator::Variable, ProgramGenerator::VariableInfo>& variableInfos)
{
	std::ostringstream vertexGlobalsString, vertexLocalsString;
	for(auto it: input.vertexInputs)
	{
		auto info = variableInfos.at(it);
		if(info.usage != ProgramGenerator::VariableInfo::Usage::kAttribute)
			vertexGlobalsString << "uniform ";
		else
			vertexGlobalsString << "in ";
		vertexGlobalsString << EmitGlslDeclaration(it, info, arraySizes);
	}

	for(auto it: input.vertexLocals)
	{
		auto info = variableInfos.at(it);
		if(strncmp(info.name.data(), "gl_", 3)) // Do not declare predefined variables
		{
			if(input.fragmentLocals.count(it))
				vertexGlobalsString << "out " << EmitGlslDeclaration(it, info, arraySizes);
			else
				vertexLocalsString << "\t" << EmitGlslDeclaration(it, info, arraySizes);
		}
	}

	std::ostringstream fragmentGlobalsString, fragmentLocalsString;

	for(auto it: input.fragmentUniforms)
	{
		auto info = variableInfos.at(it);
		fragmentGlobalsString << "uniform " << EmitGlslDeclaration(it, info, arraySizes);
	}

	for(auto it: input.fragmentLocals)
	{
		auto info = variableInfos.at(it);
		if(outputs.count(it))
			fragmentGlobalsString << "out " << EmitGlslDeclaration(it, info, arraySizes);
		else
		{
			if(input.vertexLocals.count(it))
				fragmentGlobalsString << "in " << EmitGlslDeclaration(it, info, arraySizes);
			else
				fragmentLocalsString << "\t" << EmitGlslDeclaration(it, info, arraySizes);
		}
	}

	// Passing vertex shader attributes to fragment shader:
	std::ostringstream vertexToFragmentPassingCode;
	for(auto it: input.fragmentAttributes)
	{
		auto info = variableInfos.at(it);
		fragmentGlobalsString << "in " << info.type << " vf_" << info.name << ";\n";
		vertexGlobalsString << "out " << info.type << " vf_" << info.name << ";\n";
//		fragmentLocalsString << "\t" << info.Declaration(arraySizes[*it]);
		vertexToFragmentPassingCode << "\tvf_" << info.name << " = " << info.name << ";\n";
		fragmentLocalsString << "\t" << info.name << " = vf_" << info.name << ";\n";
	}

	std::ostringstream vertexShader, fragmentShader;

	vertexShader << vertexGlobalsString.str() << std::endl;
	vertexShader << "void main()\n{\n" << vertexLocalsString.str() << "\n" << input.vertexCode << vertexToFragmentPassingCode.str() << "}\n";
	fragmentShader << fragmentGlobalsString.str() << std::endl;
	fragmentShader << "void main()\n{\n" << fragmentLocalsString.str() << "\n" << input.fragmentCode << "}\n";

	ProgramGenerator::ProgramText text;
	text.vertexShader = vertexShader.str();
	text.fragmentShader = fragmentShader.str();

//	std::cerr << vertexShader.str() << std::endl << fragmentShader.str() << std::endl;

	return text;
}

ProgramGenerator::ProgramText ProgramGenerator::GenerateProgram(
		const std::set<Variable>& inputs,
		const std::set<Variable>& outputs,
		const std::unordered_map<Variable, int>& inputArraySizes,
		bool highQuality)
{
	std::vector<Function*> functions;
	std::unordered_map<Variable, int> arraySizes = inputArraySizes;

	// Find execution paths for all outputs:
	for(auto it: outputs)
	{
		std::vector<Function*> foundFunctions = FindFunctions(inputs, it, highQuality);
		// Concatenate found functions:
		functions.insert(functions.end(), foundFunctions.begin(), foundFunctions.end());
	}

	RemoveDuplicates(functions); // Sets duplicates to nullptr

	std::ostringstream vertexCode, fragmentCode;
	ProgramEmitterInput emitterInput;

	// Functions are ordered with outputs first, so reverse:
	for(auto rit = functions.rbegin(); rit != functions.rend(); ++rit)
	{
		Function* func = *rit;
		if(!func)
			continue; // Skip duplicate filtered by RemoveDuplicates()

		// Set output array size from input array size:
		VariableMap::iterator iit = mVariableInfos.find(func->output);
		if(iit != mVariableInfos.end() && iit->second.array)
		{
			// TODO: error checking
			arraySizes[func->output] = arraySizes[func->outputArraySizeSource];
		}

		// Write function code and collect all function outputs:
		if(func->stage == Function::Stage::kVertexStage)
		{
			vertexCode << "\t" << func->source << std::endl;
			emitterInput.vertexLocals.insert(func->output);
		}
		else
		{
			fragmentCode << "\t" << func->source << std::endl;
			emitterInput.fragmentLocals.insert(func->output);
		}

		// Collect all function inputs:
		for(auto it: func->inputs)
		{
			if(func->stage == Function::Stage::kVertexStage)
			{
				if(inputs.count(it))
					emitterInput.vertexInputs.insert(it);
				else
					emitterInput.vertexLocals.insert(it);
			}
			else
			{
				if(inputs.count(it))
				{
					const VariableInfo& info = mVariableInfos.at(it);
					if(info.usage == VariableInfo::Usage::kAttribute)
					{
						// Attribute needed in fragment shader
						emitterInput.fragmentAttributes.insert(it);
						emitterInput.vertexInputs.insert(it);
						emitterInput.fragmentLocals.insert(it);
					}
					else
						emitterInput.fragmentUniforms.insert(it);
				}
				else
					emitterInput.fragmentLocals.insert(it);
			}
		}
	}

	if(emitterInput.vertexInputs.empty())
		LOG(WARNING) << "No vertex inputs used out of " << ToString(inputs);

	emitterInput.vertexCode = vertexCode.str();
	emitterInput.fragmentCode = fragmentCode.str();
	return EmitGlslProgram(
			emitterInput,
			outputs,
			arraySizes,
			mVariableInfos);
}

void ProgramGenerator::AddFunction(const Function& function)
{
	mFunctions.insert(std::make_pair(function.output, function));
}

ProgramGenerator::Variable ProgramGenerator::AddVariable(const char* name, const char* type, bool array, VariableInfo::Usage usage)
{
	return AddVariable(VariableInfo(name, type, array, usage));
}

ProgramGenerator::Variable ProgramGenerator::AddVariable(const VariableInfo& variable)
{
	Variable hash = HashUtils::MakeHash(variable.name);
//#ifndef NDEBUG
	VariableMap::iterator it = mVariableInfos.find(hash);
	if(it != mVariableInfos.end())
	{
		VariableInfo& oldVar = it->second;
		if(oldVar.name != variable.name)
			LOG(ERROR) << "Hash collision: " << oldVar.name << " vs. " << variable.name;
		if(oldVar.type != variable.type)
			throw(std::runtime_error(std::string("Existing shader variable \"") + variable.name + "\" declared with different type"));
		if(oldVar.usage != variable.usage)
			throw(std::runtime_error(std::string("Existing shader variable \"") + variable.name + "\" declared with different usage"));
	}
//#endif
	mVariableInfos[hash] = variable;
	return hash;
}

std::vector<ProgramGenerator::Function*> ProgramGenerator::FindFunctions(const std::set<Variable>& inputs, Variable output, bool highQuality)
{
	// Find functions providing the needed output:
	auto result = mFunctions.equal_range(output);

	// Sort found functions by quality, priority, number of inputs:
	std::vector<Function*> candidateFunctions;
	for(auto fIt = result.first; fIt != result.second; ++fIt)
		candidateFunctions.push_back(&(fIt->second));

	CompareFunctions comparator(highQuality);
	std::sort(candidateFunctions.begin(), candidateFunctions.end(), comparator);

	for(auto cfIt: candidateFunctions)
	{
		std::vector<Function*> functions;
		functions.push_back(cfIt);
		for(auto vIt: cfIt->inputs)
		{
			if(!inputs.count(vIt))
			{
				// Input for function not in available inputs: Find function
				std::vector<Function*> result = FindFunctions(inputs, vIt, highQuality);
				if(result.empty())
				{
					// Could not satisfy all inputs for this function candidate, try next
					functions.clear();
					break;
				}
				functions.insert(functions.end(), result.begin(), result.end());
			}
		}
		if(!functions.empty())
			return functions;
	}
	return std::vector<Function*>();
}

void ProgramGenerator::RemoveDuplicates(std::vector<Function*>& functions)
{
	std::unordered_set<Function*> seenFunctions;
	for(auto it = functions.rbegin(); it != functions.rend(); ++it)
	{
		if(!seenFunctions.insert(*it).second)
			*it = nullptr;
	}
}

std::string ProgramGenerator::ToString(const std::set<Variable>& varSet)
{
	std::ostringstream oss;
	oss << "{";
	for(auto var: varSet)
	{
		auto it = mVariableInfos.find(var);
		if(it == mVariableInfos.end())
			oss << "<unknown>, ";
		else
			oss << it->second.name << ", ";
	}
	oss << "}";
	return oss.str();
}

bool ProgramGenerator::CompareFunctions::operator() (Function* f1, Function* f2)
{
	if(f1->highQuality == f2->highQuality)
	{
		if(f1->priority == f2->priority)
			return (f1->inputs.size() > f2->inputs.size());
		else
			return (f1->priority > f2->priority);
	}
	else
	{
		if(f1->highQuality == mHighQuality)
			return true;
		else
			return false;
	}
}
