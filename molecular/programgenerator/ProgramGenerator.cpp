/*	ProgramGenerator.cpp

MIT License

Copyright (c) 2018-2020 Fabian Herb

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
#include <cassert>

#ifndef LOG
#include <iostream>
#define LOG(x) std::cerr
#endif

namespace molecular
{
namespace programgenerator
{
using namespace util;

//static std::string printFunctions(const std::vector<ProgramGenerator::Function*>& functions)
//{
//	std::stringstream functionsTrace;
//	functionsTrace << "List of functions:\n";
//	for(auto f : functions)
//	{
//		if(f == nullptr)
//			continue;
//		functionsTrace << ((f->stage == ProgramGenerator::Function::Stage::kVertexStage) ? "vertex ":
//					 ((f->stage == ProgramGenerator::Function::Stage::kFragmentStage) ? "fragment " : "geometry "));
//		functionsTrace << "prio:" << f->priority << " ";
//		functionsTrace << "hq: " << (f->highQuality ? "true " : "false ");
//		auto gsInfo = f->gsInfo;
//		if(gsInfo)
//			functionsTrace << "inprim:" << gsInfo->mInPrimitive
//						<< " outprim:" << gsInfo->mOutPrimitive
//						<< " maxvert:" << gsInfo->mMaxVertices << " ";
						
//		if(f->stage == ProgramGenerator::Function::Stage::kGeometryStage)
//			functionsTrace << "affinity:" << f->source.size() << " ";

//		functionsTrace << f->name << "(";
//		for(auto input: f->input_names)
//			functionsTrace << input << ",";
//		functionsTrace << ")\n";
//	}
//	functionsTrace << std::endl;
//	return functionsTrace.str();
//}

std::string EmitGlslDeclaration(
		Hash variable,
		const ProgramGenerator::VariableInfo& info,
		const std::unordered_map<ProgramGenerator::Variable, int>& arraySizes)
{
	std::ostringstream oss;
	oss << info.type << " " << info.name;
	if(info.array)
		oss << "[" << arraySizes.at(variable) << "]";
	return oss.str();
}

/// Input to EmitGlslProgram(), and maybe other emitters in the future
struct ProgramEmitterInput
{
	/// Pure functions source code that is used in vertex shader
	std::string vertexFunctionsCode;
	/// Pure functions source code that is used in fragment shader
	std::string fragmentFunctionsCode;
	/// Pure functions source code that is used in geometry shader
	std::string geometryFunctionsCode;
	/// Body of main() of the vertex shader without local variable declarations
	/** Consists of concatenated bodies of functions from the snippet files. */
	std::string vertexCode;

	/// Body of main() of the fragment shader without local variable declarations
	/** Consists of concatenated bodies of functions from the snippet files. */
	std::string fragmentCode;
	/// Body of main() of geometry shader
	std::vector<std::string> geometryCode;
	/// Inputs to vertex shader, both attributes and uniforms
	/** If a variable is an attribute or an uniform is decided based on information in
		ProgramGenerator::VariableInfo */
	std::unordered_set<ProgramGenerator::Variable> vertexInputs;

	/// Local variables of the vertex shader
	/** Can also become "out" variables if the same variable is used in the fragment shader. */
	std::unordered_set<ProgramGenerator::Variable> vertexLocals;

	/// Uniforms used in fragment shader
	std::unordered_set<ProgramGenerator::Variable> fragmentUniforms;

	/// Local variables in fragment shader
	/** Can also become "in" or "out" variables: If they were used as local variables in the vertex
		shader, they are declared as "out" in the vertex shader and as "in" in the fragment shader.
		If they are requested as an output of the program, they are declared as "out" in the
		fragment shader. */
	std::unordered_set<ProgramGenerator::Variable> fragmentLocals;

	/// Attributes used in fragment shader
	/** Attributes generally arrive in the vertex shader. If they are required in the fragment
		shader however, they need to be passed into it explicitly. */
	std::unordered_set<ProgramGenerator::Variable> fragmentAttributes;
	/// Geometry shader locals
	std::unordered_set<ProgramGenerator::Variable> geometryLocals;
	/// Geometry shader uniforms
	std::unordered_set<ProgramGenerator::Variable> geometryUniforms;
	/// Geometry shader info data
	/** E.g. input primitive, output primitive, etc.
	*/
	ProgramGenerator::GSInfo geometryShaderInfo;
};

/// Convert program generator output to actual GLSL text
ProgramGenerator::ProgramText EmitGlslProgram(
		const ProgramEmitterInput& input,
		const std::set<ProgramGenerator::Variable>& outputs,
		const std::unordered_map<ProgramGenerator::Variable, int>& arraySizes,
		const std::unordered_map<ProgramGenerator::Variable, ProgramGenerator::VariableInfo>& variableInfos)
{

	std::ostringstream vertexInputsString, vertexGlobalsString, vertexLocalsString;
	std::vector<std::string> vertexOutputs;
	for(auto it: input.vertexInputs)
	{
		// Inputs can either be uniforms or attributes:
		auto info = variableInfos.at(it);
		if(info.usage != ProgramGenerator::VariableInfo::Usage::kAttribute)
			vertexGlobalsString << "uniform " << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
		else
			vertexInputsString << "in " << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
	}

	for(auto it: input.vertexLocals)
	{
		auto info = variableInfos.at(it);
		if(strncmp(info.name.data(), "gl_", 3)) // Do not declare predefined variables
		{
			/* If this is also used as a local variable in the fragment shader, declare as "out".
				It is later declared as "in" in the fragment shader. */
			if(input.fragmentLocals.count(it) || input.geometryLocals.count(it))
				vertexOutputs.push_back(EmitGlslDeclaration(it, info, arraySizes));
			else
				vertexLocalsString << "\t" << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
		}
	}
	
	std::ostringstream geometryGlobalsString, geometryLocalsString,
						geometryLayout;
	std::vector<std::string> geometryOutputs;
	for(auto it : input.geometryLocals)
	{
		auto info = variableInfos.at(it);
		if(strncmp(info.name.data(), "gl_", 3))
		{
			if(input.fragmentLocals.count(it))
				geometryOutputs.push_back(EmitGlslDeclaration(it, info, arraySizes));
			else if(!input.vertexLocals.count(it))
				geometryLocalsString << "\t" << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
		}
	}
	
	for(auto it : input.geometryUniforms)
	{
		auto info = variableInfos.at(it);
		geometryGlobalsString << "uniform " << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
	}
	
	//set up geometry shader layout
	geometryLayout << "layout(" << input.geometryShaderInfo.mInPrimitive << ") in;\n";
	geometryLayout << "layout(" << input.geometryShaderInfo.mOutPrimitive <<
		", max_vertices = " << input.geometryShaderInfo.mMaxVertices << ") out;\n";

	std::ostringstream fragmentInputsString, fragmentOutputsString,
						fragmentGlobalsString, fragmentLocalsString;
	
	for(auto it: input.fragmentUniforms)
	{
		auto info = variableInfos.at(it);
		fragmentGlobalsString << "uniform " << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
	}

	for(auto it: input.fragmentLocals)
	{
		auto info = variableInfos.at(it);
		// If this is requested as an output of the program, declare as "out":
		if(outputs.count(it))
			fragmentOutputsString << "out " << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
		else
		{
			if(!(input.vertexLocals.count(it) || input.geometryLocals.count(it)))
				fragmentLocalsString << "\t" << EmitGlslDeclaration(it, info, arraySizes) << ";\n";
		}
	}
	
	// Passing vertex shader attributes to fragment shader:
	//TODO: add geometry shader support (problematic since GS source itself should be modified)
	//WARNING: this will not work if geometry shader will be enabled
	std::ostringstream vertexToFragmentPassingCode;
	for(auto it: input.fragmentAttributes)
	{
		auto info = variableInfos.at(it);
		// Declare an "in" variable (attribute name prefixed with "vf_") in fragment shader:
		fragmentGlobalsString << "in " << info.type << " vf_" << info.name << ";\n";
		// Declare same variable as "out" in vertex shader:
		vertexGlobalsString << "out " << info.type << " vf_" << info.name << ";\n";
//		fragmentLocalsString << "\t" << info.Declaration(arraySizes[*it]);
		// Assign attribute value to new "vf_" variable in vertex shader:
		vertexToFragmentPassingCode << "\tvf_" << info.name << " = " << info.name << ";\n";
		/* Declare variable with the same name as the attribute in fragment shader. Assign value
			of "vf_" variable to it: */
		fragmentLocalsString << "\t" << info.name << " = vf_" << info.name << ";\n";
	}

	// Assemble final shader text:
	std::ostringstream vertexShader, fragmentShader, geometryShader;

	//generate vertex shader
	vertexShader << vertexGlobalsString.str() << std::endl;
	vertexShader << vertexInputsString.str() << std::endl;
	std::string outVarPrefix = input.geometryShaderInfo.enabled ? "\t" : "out ";
	std::string inVarPrefix = input.geometryShaderInfo.enabled ? "\t" : "in ";
	if(input.geometryShaderInfo.enabled && !vertexOutputs.empty())
		vertexShader << "out VS_OUT {\n";
	for(const auto& out : vertexOutputs)
		vertexShader << outVarPrefix << out << ";\n";
	vertexShader << ((input.geometryShaderInfo.enabled && !vertexOutputs.empty()) ? "};\n" : "");
	vertexShader << input.vertexFunctionsCode << std::endl;
	vertexShader << "void main()\n{\n" << vertexLocalsString.str() << "\n" << input.vertexCode << vertexToFragmentPassingCode.str() << "}\n";

	//generate fragment shader
	if(input.geometryShaderInfo.enabled && !geometryOutputs.empty())
	{
		fragmentShader << "in GS_OUT {\n";
		for(const auto& in : geometryOutputs)
			fragmentShader << inVarPrefix << in << ";\n";
	} else
		for(const auto& in : vertexOutputs)
			fragmentShader << inVarPrefix << in << ";\n";

	fragmentShader << ((input.geometryShaderInfo.enabled && !geometryOutputs.empty()) ? "};\n" : "");

	fragmentShader << fragmentOutputsString.str() << std::endl;
	fragmentShader << fragmentGlobalsString.str() << std::endl;
	fragmentShader << input.fragmentFunctionsCode << std::endl;
	fragmentShader << "void main()\n{\n" << fragmentLocalsString.str() << "\n" << input.fragmentCode << "}\n";

	//generate geometry shader
	geometryShader << geometryLayout.str() << geometryGlobalsString.str() << std::endl;
	if(!vertexOutputs.empty())
		geometryShader << "in VS_OUT {\n";
	for(const auto& in : vertexOutputs)
		geometryShader << "\t" << in << ";\n";
	geometryShader << ((!vertexOutputs.empty()) ? "} gs_in[];\n" : "");

	if(!geometryOutputs.empty())
		geometryShader << "out GS_OUT {\n";
	for(const auto& out : geometryOutputs)
		geometryShader << "\t" << out << ";\n";
	geometryShader << ((!geometryOutputs.empty()) ?  "} gs_out;\n" : "");

	geometryShader << input.geometryFunctionsCode << std::endl;
	geometryShader << "void main()\n{\n" <<  "\n" << geometryLocalsString.str();
	auto primitiveDescription = input.geometryShaderInfo.primitiveDescription;
	if(!primitiveDescription.size())
		//by default EndPrimitive after all vertices are emitted
		primitiveDescription.push_back(input.geometryCode.size());
	for(size_t verticesEmitted = 0, i = 0; i < input.geometryCode.size(); i++)
	{
		geometryShader << input.geometryCode[i] << "\n";
		if(!input.geometryShaderInfo.mEnableAutoEmission)
			continue;
		
		geometryShader << "\tEmitVertex();\n";
		verticesEmitted++;
//		LOG(DEBUG) << "verticesEmitted:" << verticesEmitted << " primitiveDescription.front():" << ((primitiveDescription.size() > 0) ?
//																									 std::to_string(primitiveDescription.front()) :
//																									 std::string("empty"));
		if(primitiveDescription.size() && 
			verticesEmitted == primitiveDescription.front())
		{
			std::cout << "Ending Primitive\n";
			geometryShader << "\tEndPrimitive();\n";
			verticesEmitted = 0;
			primitiveDescription.erase(primitiveDescription.begin());
		}
	}
	geometryShader << "\n}\n";
	
	ProgramGenerator::ProgramText text;
	text.vertexShader = vertexShader.str();
	text.fragmentShader = fragmentShader.str();
	if(input.geometryShaderInfo.enabled)
		text.geometryShader = geometryShader.str();

//	LOG(DEBUG) << "VERTEX SHADER:\n" << text.vertexShader << std::endl;
//	LOG(DEBUG) << "FRAGMENT SHADER:\n" << text.fragmentShader << std::endl;
//	LOG(DEBUG) << "GEOMETRY SHADER:\n" << (input.geometryShaderInfo.enabled?text.geometryShader:"DISABLED") << std::endl;

	return text;
}

std::vector<ProgramGenerator::Function*> ProgramGenerator::FindCandidateFunctions(const Variable& candidate, bool highQuality)
{
	auto result = mFunctions.equal_range(candidate);
	// Sort found functions by quality, priority, number of inputs:
	std::vector<Function*> candidateFunctions;
	for(auto fIt = result.first; fIt != result.second; ++fIt)
		candidateFunctions.push_back(&(fIt->second));

	CompareFunctions comparator(highQuality);
	std::sort(candidateFunctions.begin(), candidateFunctions.end(), comparator);
	return candidateFunctions;
};


ProgramGenerator::ProgramText ProgramGenerator::GenerateProgram(
		const std::set<Variable>& inputs,
		const std::set<Variable>& outputs,
		const std::unordered_map<Variable, int>& inputArraySizes,
		bool highQuality)
{
	std::vector<Function*> functions;
	std::unordered_map<Variable, int> arraySizes = inputArraySizes;

	// Find execution paths for all outputs:
	size_t gsAffinity = 0;
	for(auto it: outputs)
	{
		auto foundFunctions = FindFunctions(inputs, it, highQuality, gsAffinity);
		// Concatenate found functions:
		functions.insert(functions.end(), foundFunctions.begin(), foundFunctions.end());
	}

	RemoveDuplicates(functions); // Sets duplicates to nullptr
//	LOG(DEBUG) << printFunctions(functions);

	std::ostringstream vertexCode, fragmentCode, vertexFunctionsCode, fragmentFunctionsCode, geometryFunctionsCode;
	std::vector<std::ostringstream> geometryCode;
	ProgramEmitterInput emitterInput;

	// Functions are ordered with outputs first, so reverse:
	for(auto rit = functions.rbegin(); rit != functions.rend(); ++rit)
	{
		Function* func = *rit;
		if(!func)
			continue; // Skip duplicate filtered by RemoveDuplicates()
		
		// Write pure function definition to source snippet and continue
		if(func->pureFunction)
		{
			if(func->stage == Function::Stage::kVertexStage)
				vertexFunctionsCode << func->source[0];
			else if(func->stage == Function::Stage::kVertexStage)
				fragmentFunctionsCode << func->source[0];
			else
				geometryFunctionsCode << func->source[0];
			continue;
		}
			

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
			vertexCode << "\t" << func->source[0] << std::endl;
			emitterInput.vertexLocals.insert(func->output);
		}
		else if(func->stage == Function::Stage::kFragmentStage)
		{
			fragmentCode << "\t" << func->source[0] << std::endl;
			emitterInput.fragmentLocals.insert(func->output);
		} 
		else
		{
			if(geometryCode.size() == 0)
				geometryCode.resize(func->source.size());
			assert(geometryCode.size() == func->source.size());
			for(size_t i = 0; i < geometryCode.size(); i++)
			{
				geometryCode[i] << "\t" << func->source[i] << std::endl;
			}
			emitterInput.geometryLocals.insert(func->output);
			if(func->gsInfo)
				emitterInput.geometryShaderInfo = *func->gsInfo;
			emitterInput.geometryShaderInfo.enabled = true;
		}

		// Collect all function inputs:
		for(auto it: func->inputs)
		{
			auto inputFunction = func->inputFunctions.find(it);
			if(inputFunction != func->inputFunctions.end() && inputFunction->second->pureFunction)
				continue;
			
			if(func->stage == Function::Stage::kVertexStage)
			{
				if(inputs.count(it))
					emitterInput.vertexInputs.insert(it);
				else
					emitterInput.vertexLocals.insert(it);
			}
			else if(func->stage == Function::Stage::kFragmentStage)
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
			else
			{
				if(inputs.count(it))
					emitterInput.geometryUniforms.insert(it);
				else
					emitterInput.geometryLocals.insert(it);
			}
		}
	}

	if(emitterInput.vertexInputs.empty())
		LOG(WARNING) << "No vertex inputs used out of " << ToString(inputs);

	emitterInput.vertexCode = vertexCode.str();
	emitterInput.fragmentCode = fragmentCode.str();
	emitterInput.vertexFunctionsCode = vertexFunctionsCode.str();
	emitterInput.fragmentFunctionsCode = fragmentFunctionsCode.str();
	emitterInput.geometryFunctionsCode = geometryFunctionsCode.str();
	emitterInput.geometryCode.resize(geometryCode.size());
	for(size_t i = 0; i < emitterInput.geometryCode.size(); i++)
		emitterInput.geometryCode[i] = geometryCode[i].str();
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

std::vector<ProgramGenerator::Function*> ProgramGenerator::FindFunctions(const std::set<Variable>& inputs, Variable output, bool highQuality, size_t& baseGSAffinity)
{
	struct StackItem
	{
		ProgramGenerator::Function* function;
		std::vector<ProgramGenerator::Function*> functions;
		std::vector<ProgramGenerator::Function*> candidateFunctions;
		std::vector<Variable> inputs;
		size_t gsAffinity;
	};

	auto invalidDependence = [](const std::vector<StackItem>& executionPathStack,
								ProgramGenerator::Function* function)
	{

		for(auto item : executionPathStack)
		{
			//check dependency loop
			if(item.function == function)
				return true;
			
			//check conflicting dependency
			if(item.function->stage == function->stage && 
					item.function->name == function->name)
				return true;
		}

		if(!executionPathStack.empty())
		{
			//check backward pipline dependency
			auto parrentFunction = executionPathStack.back().function;
			if(	(parrentFunction->stage == ProgramGenerator::Function::Stage::kVertexStage ||
					parrentFunction->stage == ProgramGenerator::Function::Stage::kGeometryStage) &&
					function->stage == ProgramGenerator::Function::Stage::kFragmentStage)
				return true;
			else if(parrentFunction->stage == ProgramGenerator::Function::Stage::kVertexStage &&
					function->stage == ProgramGenerator::Function::Stage::kGeometryStage)
				return true;
			
			//check dependency from fragment to vertex stage with enabled geometry stage
			if(function->stage == ProgramGenerator::Function::Stage::kVertexStage &&
					parrentFunction->stage == ProgramGenerator::Function::Stage::kFragmentStage &&
					executionPathStack.back().gsAffinity)
				return true;

			//check dependency on pure function within different pipline stage
			if(function->pureFunction && function->stage != executionPathStack.back().function->stage)
				return true;
		}
		
		
		return false;
	};

	std::vector<StackItem> executionPathStack;
	StackItem currentState = {nullptr, {}, FindCandidateFunctions(output, highQuality), {}, baseGSAffinity};
	while(true)
	{
		assert(currentState.functions.empty() || executionPathStack.empty());
		if(currentState.candidateFunctions.empty())
		{
			if(!executionPathStack.empty())
			{
				// All candidates for this input discarded, thus the parrent function failed to find a candidate for its input.
				// Start processing the next candidate for a parrent
				currentState = std::move(executionPathStack.back());
				currentState.functions.clear();
				executionPathStack.pop_back();
				continue;
			} else
				// We are back to the root. Finish processing execution path tree
				break;
		}

		// Before processing candidate function restore its geometry stage affinity
		if(!executionPathStack.empty())
			// Inherit gsAffinity from parrent
			currentState.gsAffinity = executionPathStack.back().gsAffinity;
		else
			// It is a root of the tree. Set its gs affinity to initial value
			currentState.gsAffinity = baseGSAffinity;

		currentState.function = currentState.candidateFunctions.front();
		currentState.candidateFunctions.erase(currentState.candidateFunctions.begin());
		
		// Handle invalid dependency. If detected, check next candidate
		if(invalidDependence(executionPathStack, currentState.function))
			continue;
		
		// Check GS affinity if not pure function
		if(!currentState.function->pureFunction)
		{
			if(currentState.gsAffinity != 0 && 
				currentState.function->stage == Function::Stage::kGeometryStage &&
				currentState.function->source.size() != currentState.gsAffinity)
				//This function is not aligned with general geometry shader affinity (number of vertex outputs)
				continue;
			else if(currentState.gsAffinity == 0 && 
					currentState.function->stage == Function::Stage::kGeometryStage)
				//in case if it is first geometry stage function met on the path, make affinity fit number of sources
				currentState.gsAffinity = currentState.function->source.size();
		}
		
		currentState.inputs = currentState.function->inputs;
		currentState.functions = {currentState.function};
		while(true)
		{

			if(currentState.inputs.empty())
			{
				// Finish processing of inputs				

				if(executionPathStack.empty())
				{
					if(!currentState.functions.empty())
						// We are back to a root function, and execution path is found. 
						// Finish tree traversal by clearing all candidate functions
						currentState.candidateFunctions.clear();
					break;
				}
				
				if(!currentState.functions.empty())
				{
					// This trunk has acceptable dependencys, 
					// thus pass all found functions to parrent node and 
					// continue processing other parrent inputs 
					executionPathStack.back().functions.insert(executionPathStack.back().functions.end(),
																currentState.functions.begin(),
																currentState.functions.end());
					executionPathStack.back().gsAffinity = currentState.gsAffinity;
					executionPathStack.back().function->inputFunctions[currentState.function->output] = currentState.function;
					currentState = std::move(executionPathStack.back());
					executionPathStack.pop_back();
					continue;
					
				} else
					// Current candidate has no valid dependency chain, process next candidate
					break;
			}

			// Process next input
			auto input = currentState.inputs.front();
			currentState.inputs.erase(currentState.inputs.begin());

			if(!inputs.count(input))
			{
				auto newCandidateFunctions = FindCandidateFunctions(input, highQuality);
				if(!newCandidateFunctions.empty())
				{
					// Push current state and start processing new trunk
					executionPathStack.push_back(currentState);
					currentState = StackItem();
					currentState.gsAffinity = executionPathStack.back().gsAffinity;
					currentState.candidateFunctions = std::move(newCandidateFunctions);
					break;
				} else
				{
					// This input is not in shader-inputs, and has no candidates. Process next candidate
					currentState.functions.clear();
					break;
				}
			}
			// Exit loops at the beginnig
		}
	}
	
	assert(executionPathStack.empty());
	baseGSAffinity = currentState.gsAffinity;
	return currentState.functions;
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

} // namespace programgenerator
} // namespace molecular
