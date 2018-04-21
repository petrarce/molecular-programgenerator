/*	ProgramFile.cpp

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

#include "ProgramFile.h"
#include "util/Parser.h"
#include <stdlib.h>

using namespace Parser;

bool ProgramFile::Parse(char* begin, char* end)
{
	// Brainfuck...
	typedef Concatenation<Alpha, Repetition<Alternation<Alpha, Digit, Char<'_'>> > > Identifier;
	typedef Action<Concatenation<Char<'f'>, Char<'r'>, Char<'a'>, Char<'g'>, Char<'m'>, Char<'e'>, Char<'n'>, Char<'t'> >, kFragmentStage> Fragment;
	typedef Action<Concatenation<Char<'v'>, Char<'e'>, Char<'r'>, Char<'t'>, Char<'e'>, Char<'x'> >, kVertexStage> Vertex;
	typedef Action<Concatenation<Char<'l'>, Char<'o'>, Char<'w'>, Char<'_'>, Char<'q'> >, kLowQuality> LowQ;
	typedef Concatenation<Char<'p'>, Char<'r'>, Char<'i'>, Char<'o'>, Char<'='>, Action<Integer, kPriority> > Prio;
	typedef Concatenation<Char<'a'>, Char<'t'>, Char<'t'>, Char<'r'> > Attr;
	typedef Concatenation<Char<'o'>, Char<'u'>, Char<'t'> > Out;
	typedef Concatenation<Alternation<Fragment, Vertex, LowQ, Prio>, Whitespace> Attribute;

	typedef Concatenation<
			Option<Alternation<Action<Concatenation<Attr, Whitespace>, kAttribute >, Action<Concatenation<Out, Whitespace>, kOutput > > >,
			Action<Identifier, kType>,
			Option<Action<Concatenation<Char<'['>, Char<']'> >, kArray> > > Type;

	typedef Concatenation<
			Option<Whitespace>,
			Type,
			Whitespace,
			Action<Identifier, kParameterName>,
			Option<Whitespace> > Parameter;

	typedef Concatenation<
			Type,
			Whitespace,
			Action<Identifier, kFunctionName>,
			Option<Whitespace>,
			Char<'('>,
			Option<Concatenation<Parameter, Repetition<Concatenation<Char<','>, Parameter> > > >,
			Char<')'> > Declaration;

	typedef Concatenation<
			Option<Whitespace>,
			Repetition<Attribute>,
			Declaration,
			Option<Whitespace>,
			Char<'{'>,
			Action<Body, kBody>,
			Option<Whitespace>,
			Char<'}'>
			> Function;
	typedef Concatenation<Repetition<Function>, Option<Whitespace>, Option<Char<0> > > File;

	bool success = File::Parse(begin, end, this);

	if(success && begin == end)
		return true;
	else
		return false;
}

void ProgramFile::ParserAction(int action, char* begin, char* end)
{
	char tmp;
	switch(action)
	{
	case kPriority:
		tmp = *end;
		*end = 0;
		mCurrentFunction.priority = strtol(begin, nullptr, 10);
		*end = tmp;
		break;

	case kLowQuality:
		mCurrentFunction.highQuality = false;
		break;

	case kVertexStage:
		mCurrentFunction.stage = ProgramGenerator::Function::Stage::kVertexStage;
		break;

	case kFragmentStage:
		mCurrentFunction.stage = ProgramGenerator::Function::Stage::kFragmentStage;
		break;

	case kType:
		mCurrentVariable.type = std::string(begin, end);
		break;

	case kAttribute:
		mCurrentVariable.usage = ProgramGenerator::VariableInfo::Usage::kAttribute;
		break;

	case kOutput:
		mCurrentVariable.usage = ProgramGenerator::VariableInfo::Usage::kOutput;
		break;

	case kArray:
		mCurrentVariable.array = true;
		break;

	case kFunctionName:
		mCurrentFunction.output = HashUtils::MakeHash(begin, end);
		mCurrentVariable.name = std::string(begin, end);
		mVariables.push_back(mCurrentVariable);
		mCurrentVariable = ProgramGenerator::VariableInfo(); // Restore defaults
		break;

	case kParameterName:
		mCurrentFunction.inputs.push_back(HashUtils::MakeHash(begin, end));
		mCurrentVariable.name = std::string(begin, end);
		mVariables.push_back(mCurrentVariable);
		mCurrentVariable = ProgramGenerator::VariableInfo(); // Restore defaults
		break;

	case kBody:
		mCurrentFunction.source = std::string(begin, end);
		mFunctions.push_back(mCurrentFunction);
		mCurrentFunction = ProgramGenerator::Function(); // Restore defaults
		break;
	}
}
