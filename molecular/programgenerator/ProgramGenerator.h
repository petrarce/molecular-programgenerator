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

	/// Information about a function
	/** @see CompareFunctions */
	struct Function
	{
		enum class Stage
		{
			kVertexStage,
			kFragmentStage
		};

		std::vector<Variable> inputs;
		std::string source;
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
	};

	/// Output of the program generator
	struct ProgramText
	{
		std::string vertexShader;
		std::string fragmentShader;
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

	/// Recursively find functions that provide a given output
	std::vector<Function*> FindFunctions(const std::set<Variable>& inputs, Variable output, bool highQuality);

	/// Set duplicate functions to nullptr
	static void RemoveDuplicates(std::vector<Function*>& functions);

	/** Only for debugging. */
	std::string ToString(const std::set<Variable>& varSet);

	/// Functor that compares two Function objects
	class CompareFunctions : public std::binary_function<bool, Function*,Function*>
	{
	public:
		CompareFunctions(bool highQuality) : mHighQuality(highQuality) {}
		bool operator() (Function* f1, Function* f2);

	private:
		bool mHighQuality;
	};

	/// Maps outputs to functions
	FunctionMap mFunctions;
	VariableMap mVariableInfos;
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
