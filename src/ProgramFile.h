/*	ProgramFile.h

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

#ifndef PROGRAMFILE_H
#define PROGRAMFILE_H

#include "ProgramGenerator.h"
#include <list>

namespace molecular
{

/// Reads configuration files for the ProgramGenerator
/** @code
	digit = '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' ;
	character = 'a' | 'b' ... 'z' | 'A' | 'B' ... 'Z' ;
	number = [ '-' ], digit, { digit } ;
	identifier = character, { character | digit } ;
	parameter = [whitespace], identifier, whitespace, identifier, [whitespace] ;
	attribute = 'fragment' | 'vertex' | 'low_q' | 'prio=', number ;
	body = '{', ?text with balanced parantheses?, '}' ;
	function = [whitespace], {attribute, whitespace}, identifier, whitespace, identifier, [whitespace], '(', [parameter, {',', parameter}], ')', [whitespace], body ;
	@endcode
*/
class ProgramFile
{
public:
	ProgramFile(char* begin, char* end)
	{
		if(!Parse(begin, end))
			throw std::runtime_error("Parse error");
	}

	typedef std::vector<ProgramGenerator::Function> FunctionContainer;
	typedef std::vector<ProgramGenerator::VariableInfo> VariableContainer;

	const FunctionContainer& GetFunctions() const {return mFunctions;}
	const VariableContainer& GetVariables() const {return mVariables;}

	void ParserAction(int action, char* begin, char* end);

private:
	enum
	{
		kPriority,
		kFragmentStage,
		kVertexStage,
		kLowQuality,
		kAttribute,
		kOutput,
		kArray,
		kType,
		kFunctionName,
		kParameterName,
		kBody
	};

	class Body
	{
	public:
		template<class Iterator>
		static bool Parse(Iterator& begin, Iterator end, void*)
		{
			if(begin == end)
				return false;

			Iterator oldBegin = begin;

			int parensCount = 1;
			while(begin != end)
			{
				if(*begin == '{')
					parensCount++;
				else if(*begin == '}')
					parensCount--;

				// Don't increment here

				if(parensCount == 0)
					return true;

				// Increment here to keep surrounding closing bracket in buffer:
				begin++;
			}
			begin = oldBegin;
			return false;
		}
	};

	bool Parse(char* begin, char* end);

	ProgramGenerator::Function mCurrentFunction;
	ProgramGenerator::VariableInfo mCurrentVariable;

	FunctionContainer mFunctions;
	VariableContainer mVariables;
};

} // namespace
#endif // PROGRAMFILE_H
