/*	ProgramGenerator.h

MIT License

Copyright (c) 2018-2019 Fabian Herb

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

#ifndef MOLECULAR_PROGRAMGENERATOR_H
#define MOLECULAR_PROGRAMGENERATOR_H

#include <vector>
#include <set>
#include <unordered_map>
#include <map>
#include <molecular/util/Hash.h>
#include <memory>

namespace molecular
{
namespace programgenerator
{

/// Generates shader programs from a given set of inputs and outputs
class ProgramGenerator
{
public:
	typedef util::Hash Variable;

	/// Information about a variable
	struct VariableInfo
	{
		enum class Usage
		{
			kUniformOrLocal,
			kAttribute,
			kOutput
		};

		VariableInfo() = default;
		VariableInfo(const char* name, const char* type, bool array = false, Usage usage = Usage::kUniformOrLocal) :
			name(name), type(type), usage(usage), array(array) {}

		std::string name;
		std::string type;
		Usage usage = Usage::kUniformOrLocal;
		bool array = false;
	};

	///Geometry shader information
	struct GSInfo {
		///Input primitive type. See https://www.khronos.org/opengl/wiki/Geometry_Shader for possible variants
		std::string mInPrimitive {"points"};
		///Output primitive type. See https://www.khronos.org/opengl/wiki/Geometry_Shader for possible variants
		std::string mOutPrimitive {"points"};
		/// Maximum number of vertices that will be written by a single invocation of the GS
		size_t mMaxVertices {1};
		/// Description of the primitive for automatic EmitVertex/EndPrimitive declaration
		/** Each value shows how many vertices should be emitted before each EndPrimitive()*/
		std::vector<size_t> primitiveDescription;
		/// State variable. Determines if geometry shader turned on/off
		bool enabled {false};
		///State variable. Determines if automatic EmitVertex/EndPrimitive is enabled
		bool mEnableAutoEmission {true};
		//TODO: add streaming and instancing support
	};

	/// Information about a function
	/** @see CompareFunctions */
	struct Function
	{
		enum class Stage
		{
			kVertexStage,
			kFragmentStage,
			kGeometryStage,
		};

		std::vector<Variable> inputs;
		/// Input to function mapping, computed during dependency resolution
		std::map<Variable, Function*> inputFunctions;
		/// Source code for of the function. 
		/** For Geometry shader, it is allowed to have multiple body declaration.
			Generator will append all snippets and correctly generate EndVertex/EndPrimitive 
			for each snippet*/
		std::vector<std::string> source;
		Variable output = 0;

		/// Input variable from which the array size of the output variable is derived
		Variable outputArraySizeSource = 0;
		Stage stage = Stage::kVertexStage;

		/// Priority among functions providing the same output as this one
		/** If there is more than one function providing same output while all inputs are available,
			the one with the highest priority wins.
			@see CompareFunctions */
		int priority = 0;

		/// Simple quality selector
		bool highQuality = true;
		
		/// Determines if this function is pure function
		bool pureFunction = false;

		//Geometry shader information
		std::shared_ptr<GSInfo> gsInfo;
		
		/// For debug purposes
		std::string name;
		std::vector<std::string> input_names;
	};

	/// Output of the program generator
	struct ProgramText
	{
		std::string vertexShader;
		std::string fragmentShader;
		std::string geometryShader;
	};

	/// Generate program from separate inputs and outputs
	ProgramText GenerateProgram(const std::set<Variable>& inputs,
			const std::set<Variable>& outputs,
			const std::unordered_map<Variable, int>& arraySizes = std::unordered_map<Variable, int>(),
			bool highQuality = true);

	/// Generate program from collection of variables (inputs and outputs)
	/** Variables are sorted first by querying their VariableInfo. */
	template<class Iterator>
	ProgramText GenerateProgram(
			Iterator varsBegin, Iterator varsEnd,
			const std::unordered_map<Variable, int>& arraySizes = std::unordered_map<Variable, int>(),
			bool highQuality = true);

	/// Add a function to be considered in program generation
	void AddFunction(const Function &function);
	Variable AddVariable(const char* name, const char* type, bool array = false, VariableInfo::Usage usage = VariableInfo::Usage::kUniformOrLocal);
	Variable AddVariable(const VariableInfo& variable);

private:
	typedef std::unordered_map<Variable, VariableInfo> VariableMap;
	typedef std::multimap<Variable, Function> FunctionMap;

	/// Find alternatives for a given candidate
	std::vector<ProgramGenerator::Function*> FindCandidateFunctions(const Variable& candidate, bool highQuality);
	/// Find functions that provide a given output
	std::vector<ProgramGenerator::Function*> FindFunctions(const std::set<Variable>& inputs, Variable output, bool highQuality, size_t& baseGSAffinity);


	/// Set duplicate functions to nullptr
	static void RemoveDuplicates(std::vector<Function*>& functions);

	/** Only for debugging. */
	std::string ToString(const std::set<Variable>& varSet);

	/// Functor that compares two Function objects
	class CompareFunctions
	{
	public:
		using first_argument_type = bool;
		using second_argument_type = Function*;
		using result_type = Function*;

		CompareFunctions(bool highQuality) : mHighQuality(highQuality) {}
		bool operator() (Function* f1, Function* f2);

	private:
		bool mHighQuality;
	};

	/// Maps outputs to functions
	FunctionMap mFunctions;
	VariableMap mVariableInfos;
	GSInfo mGeometryShaderInfo;
};

template<class Iterator>
ProgramGenerator::ProgramText ProgramGenerator::GenerateProgram(
		Iterator varsBegin, Iterator varsEnd,
		const std::unordered_map<Variable, int>& arraySizes, bool highQuality)
{
	// Separate inputs and outputs:
	std::set<Variable> inputs, outputs;
	for(Iterator it = varsBegin; it != varsEnd; ++it)
	{
		if(mVariableInfos[*it].usage == VariableInfo::Usage::kOutput)
			outputs.insert(*it);
		else
			inputs.insert(*it);
	}
	return GenerateProgram(inputs, outputs, arraySizes, highQuality);
}

} // namespace programgenerator
} // namespace molecular

#endif // MOLECULAR_PROGRAMGENERATOR_H
